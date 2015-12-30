#include "stdafx.h"
#include "dbdiff.h"
#include "StringUtils.h"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace dbdiff
{
	CDiffEngine::CDiffEngine()
	{
	}

	CDiffEngine::~CDiffEngine()
	{
	}

	bool CDiffEngine::Process(CDBXSchemaPtr pInput, SchemaPtr& pOutput)
	{
		CDiffEngine engine;

		try
		{
			engine.ProcessImpl(pInput, pOutput);
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	void CDiffEngine::ProcessImpl(CDBXSchemaPtr pInput, SchemaPtr& pOutput)
	{
		auto verList = CollectVersionList(pInput);

		std::vector<DBDiffPtr> dbDiffList;

		std::for_each(pInput->m_dbList.begin(), pInput->m_dbList.end(), [&](const CDBXDatabasePtr& pDB)
		{
			DBDiffPtr pDBDiff;
			ProcessDB(pDB, verList, pDBDiff);
			dbDiffList.push_back(pDBDiff);
		});

		auto pResult = std::make_shared<Schema>();
		pResult->m_majorVer = pInput->majorVer;


		for (auto i = 0; i != dbDiffList.size(); i++)
		{
			auto pDBDiff = dbDiffList.at(i);

			if (pDBDiff->diffInfoList.empty())
			{
				ThrowError();
			}

			if (pDBDiff->diffInfoList.at(0)->newTableList.empty())
			{
				ThrowError();
			}

			int ver = pDBDiff->diffInfoList.at(0)->version;

			auto pDatabase = EnsureDatabase(pResult, ver, pDBDiff->name);

			for (auto j = 0; j != pDBDiff->diffInfoList.size(); j++)
			{
				auto pDiffInfo = pDBDiff->diffInfoList.at(j);

				for (auto k = 0; k != pDiffInfo->newTableList.size(); k++)
				{
					auto pTableInfo = pDiffInfo->newTableList.at(k);

					auto pTable = EnsureTable(pDatabase, pDiffInfo->version, pTableInfo->name);

					auto pColumnRange = EnsureColumnRange(pTable, pDiffInfo->version);

					for (auto l = 0; l != pTableInfo->columnList.size(); l++)
					{
						auto pColumn = std::make_shared<Column>();
						pColumn->m_name = pTableInfo->columnList.at(l).name;
						pColumnRange->m_columnList.push_back(pColumn);
					}
				}

				for (auto k = 0; k != pDiffInfo->appendTableList.size(); k++)
				{
					auto pTableInfo = pDiffInfo->appendTableList.at(k);

					auto pTable = FindTable(pDatabase, pTableInfo->name);

					auto pColumnRange = EnsureColumnRange(pTable, pDiffInfo->version);

					for (auto l = 0; l != pTableInfo->columnList.size(); l++)
					{
						auto pColumn = std::make_shared<Column>();
						pColumn->m_name = pTableInfo->columnList.at(l).name;
						pColumnRange->m_columnList.push_back(pColumn);
					}
				}
			}
		}

		pOutput.swap(pResult);
	}

	void CDiffEngine::ProcessDB(CDBXDatabasePtr pDB, const std::vector<int>& verList, DBDiffPtr& pDBDiff)
	{
		auto pConn = std::make_shared<CSQLWConnection>();
		int r = pConn->Open(L":memory:");
		if (r != SQLITE_OK)
		{
			ThrowError();
		}

		auto pDBDiff1 = std::make_shared<DBDiff>();
		pDBDiff1->name = CStringUtils::ConvertWideToUtf8(pDB->m_name);

		std::vector<DBInfoPtr> dbInfoList;

		for (auto i = 0; i != verList.size(); i++)
		{
			ProcessDBVer(pConn, pDB, verList.at(i), dbInfoList);
		}

		if (!dbInfoList.empty())
		{
			auto di = DiffDB(std::make_shared<DBInfo>(), dbInfoList.at(0));
			pDBDiff1->diffInfoList.push_back(di);
			
			for (size_t i = 1; i < dbInfoList.size(); i++)
			{
				di = DiffDB(dbInfoList.at(i - 1), dbInfoList.at(i));
				pDBDiff1->diffInfoList.push_back(di);
			}
		}

		pDBDiff.swap(pDBDiff1);
	}

	void CDiffEngine::ProcessDBVer(CSQLWConnectionPtr pConn, CDBXDatabasePtr pDB, int version, std::vector<DBInfoPtr>& dbInfoList)
	{
		CDBXPatchPtr pPatch;
		for (auto i = 0; i != pDB->m_patchList.size(); i++)
		{
			if (pDB->m_patchList.at(i)->m_version == version)
			{
				pPatch = pDB->m_patchList.at(i);
				break;
			}
		}

		if (pPatch == nullptr)
		{
			return;
		}

		for (auto i = 0; i != pPatch->m_sqlList.size(); i++)
		{
			auto sql = pPatch->m_sqlList.at(i);
			int r = pConn->Execute(sql.c_str(), sql.size(), nullptr);
			if (r != SQLITE_DONE)
			{
				ThrowError();
			}
		}

		auto tableNameList = GetTableNames(pConn);

		auto pDBInfo = std::make_shared<DBInfo>();
		pDBInfo->version = version;

		for (auto i = 0; i != tableNameList.size(); i++)
		{
			auto pTableInfo = GetTableInfo(pConn, tableNameList.at(i));
			pDBInfo->tableList.push_back(pTableInfo);
		}

		std::sort(pDBInfo->tableList.begin(), pDBInfo->tableList.end(),
			[](const TableInfoPtr& p1, const TableInfoPtr& p2)
		{
			return p1->name < p2->name;
		});

		RebuildIndex(pDBInfo);

		dbInfoList.push_back(pDBInfo);
	}

	std::vector<std::string> CDiffEngine::GetTableNames(CSQLWConnectionPtr pConn)
	{
		CSQLWStatementPtr pStmt;
		int r = pConn->Prepare("SELECT name from sqlite_master WHERE type='table'", -1, nullptr, pStmt);
		if (r != SQLITE_OK)
		{
			ThrowError();
		}

		std::vector<std::string> result;

		for (;;)
		{
			r = pStmt->Step();
			if (r == SQLITE_ROW)
			{
				result.push_back(pStmt->GetColumnText(0));
			}
			else if (r == SQLITE_DONE)
			{
				break;
			}
			else
			{
				ThrowError();
			}
		}

		return result;
	}

	CDiffEngine::TableInfoPtr CDiffEngine::GetTableInfo(CSQLWConnectionPtr pConn, const std::string& tableName)
	{
		auto pResult = std::make_shared<TableInfo>();
		pResult->name = tableName;

		std::string sql = "PRAGMA table_info(" + tableName + ")";
		CSQLWStatementPtr pStmt;
		int r = pConn->Prepare(sql.c_str(), -1, nullptr, pStmt);
		if (r != SQLITE_OK)
		{
			ThrowError();
		}

		for (;;)
		{
			r = pStmt->Step();
			if (r == SQLITE_ROW)
			{
				ColumnInfo info;
				info.name = pStmt->GetColumnText(1);
				pResult->columnList.push_back(info);
			}
			else if (r == SQLITE_DONE)
			{
				break;
			}
			else
			{
				ThrowError();
			}
		}

		return pResult;
	}

	CDiffEngine::DiffInfoPtr CDiffEngine::DiffDB(const DBInfoPtr& p1, const DBInfoPtr& p2)
	{
		auto pResult = std::make_shared<DiffInfo>();
		pResult->version = p2->version;

		if (p1->tableList.size() > p2->tableList.size())
		{
			ThrowError();
		}

		if (p1->tableList.size() == p2->tableList.size())
		{
			for (auto i = 0; i != p1->tableList.size(); i++)
			{
				auto table1 = p1->tableList.at(i);
				auto table2 = p2->tableList.at(i);

				if (table1->name != table2->name)
				{
					ThrowError();
				}

				if (table1->columnList.size() != table2->columnList.size())
				{
					auto ti = std::make_shared<TableInfo>();
					ti->name = table1->name;
					ti->columnList.assign(table2->columnList.data() + table1->columnList.size(), table2->columnList.data() + table2->columnList.size());
					pResult->appendTableList.push_back(ti);
				}
			}
		}
		else
		{
			for (auto i = 0; i != p2->tableList.size(); i++)
			{
				auto table2 = p2->tableList.at(i);
				auto it = p1->tableIndex.find(table2->name);
				if (it == p1->tableIndex.end())
				{
					pResult->newTableList.push_back(table2);
				}
				else
				{
					auto table1 = p1->tableList.at(it->second);

					if (table1->name != table2->name)
					{
						ThrowError();
					}

					if (table1->columnList.size() != table2->columnList.size())
					{
						auto ti = std::make_shared<TableInfo>();
						ti->name = table1->name;
						ti->columnList.assign(table2->columnList.data() + table1->columnList.size(), table2->columnList.data() + table2->columnList.size());
						pResult->appendTableList.push_back(ti);
					}
				}
			}

		}

		return pResult;
	}

	DatabasePtr CDiffEngine::EnsureDatabase(SchemaPtr pSchema, int ver, const std::string& name)
	{
		auto it = std::find_if(pSchema->m_databaseRangeList.begin(), pSchema->m_databaseRangeList.end(), [=](const DatabaseRangePtr& p)
		{
			return p->m_version == ver;
		});

		DatabaseRangePtr pDatabaseRange;

		if (it == pSchema->m_databaseRangeList.end())
		{
			pDatabaseRange = std::make_shared<DatabaseRange>();
			pDatabaseRange->m_version = ver;
			pSchema->m_databaseRangeList.push_back(pDatabaseRange);
			std::sort(pSchema->m_databaseRangeList.begin(), pSchema->m_databaseRangeList.end(), [](const DatabaseRangePtr& p1, const DatabaseRangePtr& p2)
			{
				return p1->m_version < p2->m_version;
			});
		}
		else
		{
			pDatabaseRange = *it;
		}

		auto it2 = std::find_if(pDatabaseRange->m_databaseList.begin(), pDatabaseRange->m_databaseList.end(), [=](const DatabasePtr& p)
		{
			return p->m_name == name;
		});

		DatabasePtr pDatabase;

		if (it2 == pDatabaseRange->m_databaseList.end())
		{
			pDatabase = std::make_shared<Database>();
			pDatabase->m_name = name;
			pDatabaseRange->m_databaseList.push_back(pDatabase);
		}
		else
		{
			pDatabase = *it2;
		}

		return pDatabase;
	}

	TablePtr CDiffEngine::EnsureTable(DatabasePtr pDatabase, int ver, const std::string& name)
	{
		auto it = std::find_if(pDatabase->m_tableRangeList.begin(), pDatabase->m_tableRangeList.end(), [=](const TableRangePtr& p)
		{
			return p->m_version == ver;
		});

		TableRangePtr pTableRange;

		if (it == pDatabase->m_tableRangeList.end())
		{
			pTableRange = std::make_shared<TableRange>();
			pTableRange->m_version = ver;
			pDatabase->m_tableRangeList.push_back(pTableRange);
			std::sort(pDatabase->m_tableRangeList.begin(), pDatabase->m_tableRangeList.end(), [](const TableRangePtr& p1, const TableRangePtr& p2)
			{
				return p1->m_version < p2->m_version;
			});
		}
		else
		{
			pTableRange = *it;
		}

		auto it2 = std::find_if(pTableRange->m_tableList.begin(), pTableRange->m_tableList.end(), [=](const TablePtr& p)
		{
			return p->m_name == name;
		});

		TablePtr pTable;

		if (it2 == pTableRange->m_tableList.end())
		{
			pTable = std::make_shared<Table>();
			pTable->m_name = name;
			pTableRange->m_tableList.push_back(pTable);
		}
		else
		{
			pTable = *it2;
		}

		return pTable;
	}

	TablePtr CDiffEngine::FindTable(DatabasePtr pDatabase, const std::string& name)
	{
		for (auto i = 0; i != pDatabase->m_tableRangeList.size(); i++)
		{
			auto pTableRange = pDatabase->m_tableRangeList.at(i);

			for (auto j = 0; j != pTableRange->m_tableList.size(); j++)
			{
				auto pTable = pTableRange->m_tableList.at(j);
				if (pTable->m_name == name)
				{
					return pTable;
				}
			}
		}

		ThrowError();

		// C4715
		return nullptr;
	}

	ColumnRangePtr CDiffEngine::EnsureColumnRange(TablePtr pTable, int ver)
	{
		auto it = std::find_if(pTable->m_columnRangeList.begin(), pTable->m_columnRangeList.end(), [=](const ColumnRangePtr& p)
		{
			return p->m_version == ver;
		});

		ColumnRangePtr pColumnRange;

		if (it == pTable->m_columnRangeList.end())
		{
			pColumnRange = std::make_shared<ColumnRange>();
			pColumnRange->m_version = ver;
			pTable->m_columnRangeList.push_back(pColumnRange);
			std::sort(pTable->m_columnRangeList.begin(), pTable->m_columnRangeList.end(), [](const ColumnRangePtr& p1, const ColumnRangePtr& p2)
			{
				return p1->m_version < p2->m_version;
			});
		}
		else
		{
			pColumnRange = *it;
		}

		return pColumnRange;
	}

	void CDiffEngine::ThrowError()
	{
		throw std::runtime_error("Error!");
	}

	std::vector<int> CDiffEngine::CollectVersionList(CDBXSchemaPtr pSchema)
	{
		std::set<int> verSet;
		std::vector<int> verVec;

		for (auto i = 0; i != pSchema->m_dbList.size(); i++)
		{
			auto pDB = pSchema->m_dbList.at(i);

			for (auto j = 0; j != pDB->m_patchList.size(); j++)
			{
				auto ver = pDB->m_patchList.at(j)->m_version;
				if (verSet.find(ver) == verSet.end())
				{
					verSet.insert(ver);
					verVec.push_back(ver);
				}
			}
		}

		std::sort(verVec.begin(), verVec.end());
		return verVec;
	}

	void CDiffEngine::RebuildIndex(const DBInfoPtr& p)
	{
		p->tableIndex.clear();

		for (auto i = 0; i != p->tableList.size(); i++)
		{
			p->tableIndex.insert(std::make_pair(p->tableList.at(i)->name, i));
		}
	}
}
