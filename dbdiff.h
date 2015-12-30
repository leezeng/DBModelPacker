#ifndef __dbdiff_h__
#define __dbdiff_h__

#include "dbx.h"
#include "SQLiteWrapper.h"
#include <map>
#include <string>
#include <vector>

namespace dbdiff
{
	class Schema;
	class DatabaseRange;
	class Database;
	class TableRange;
	class Table;
	class ColumnRange;
	class Column;

	typedef std::shared_ptr<Schema> SchemaPtr;
	typedef std::shared_ptr<DatabaseRange> DatabaseRangePtr;
	typedef std::shared_ptr<Database> DatabasePtr;
	typedef std::shared_ptr<TableRange> TableRangePtr;
	typedef std::shared_ptr<Table> TablePtr;
	typedef std::shared_ptr<ColumnRange> ColumnRangePtr;
	typedef std::shared_ptr<Column> ColumnPtr;

	class Schema
	{
	public:
		int m_majorVer;
		std::string m_listCodeName;
		std::vector<DatabaseRangePtr> m_databaseRangeList;
	};

	class DatabaseRange
	{
	public:
		std::string m_codeName;
		int m_version;
		std::string m_listCodeName;
		std::vector<DatabasePtr> m_databaseList;
	};

	class Database
	{
	public:
		std::string m_name;
		std::string m_listCodeName;
		std::vector<TableRangePtr> m_tableRangeList;
	};

	class TableRange
	{
	public:
		std::string m_codeName;
		int m_version;
		std::string m_listCodeName;
		std::vector<TablePtr> m_tableList;
	};

	class Table
	{
	public:
		std::string m_name;
		std::string m_listCodeName;
		std::vector<ColumnRangePtr> m_columnRangeList;
	};

	class ColumnRange
	{
	public:
		std::string m_codeName;
		int m_version;
		std::string m_listCodeName;
		std::vector<ColumnPtr> m_columnList;
	};

	class Column
	{
	public:
		std::string m_name;
	};

	class CDiffEngine
	{
	public:
		static bool Process(CDBXSchemaPtr pInput, SchemaPtr& pOutput);

	private:
		CDiffEngine();
		~CDiffEngine();

		struct ColumnInfo
		{
			std::string name;
		};

		struct TableInfo
		{
			std::string name;
			std::vector<ColumnInfo> columnList;
		};

		typedef std::shared_ptr<TableInfo> TableInfoPtr;

		struct DBInfo
		{
			int version;
			std::vector<TableInfoPtr> tableList;
			std::map<std::string, size_t> tableIndex;
		};

		typedef std::shared_ptr<DBInfo> DBInfoPtr;

		struct DiffInfo
		{
			int version;
			std::vector<TableInfoPtr> newTableList;
			std::vector<TableInfoPtr> appendTableList;
		};

		typedef std::shared_ptr<DiffInfo> DiffInfoPtr;

		struct DBDiff
		{
			std::string name;
			std::vector<DiffInfoPtr> diffInfoList;
		};

		typedef std::shared_ptr<DBDiff> DBDiffPtr;

		void ProcessImpl(CDBXSchemaPtr pInput, SchemaPtr& pOutput);
		void ProcessDB(CDBXDatabasePtr pDB, const std::vector<int>& verList, DBDiffPtr& pDBDiff);
		void ProcessDBVer(CSQLWConnectionPtr pConn, CDBXDatabasePtr pDB, int version, std::vector<DBInfoPtr>& dbInfoList);
		std::vector<std::string> GetTableNames(CSQLWConnectionPtr pConn);
		TableInfoPtr GetTableInfo(CSQLWConnectionPtr pConn, const std::string& tableName);
		DiffInfoPtr DiffDB(const DBInfoPtr& p1, const DBInfoPtr& p2);

		DatabasePtr EnsureDatabase(SchemaPtr pSchema, int ver, const std::string& name);
		TablePtr EnsureTable(DatabasePtr pDatabase, int ver, const std::string& name);
		TablePtr FindTable(DatabasePtr pDatabase, const std::string& name);
		ColumnRangePtr EnsureColumnRange(TablePtr pTable, int ver);

		void ThrowError();

		static std::vector<int> CollectVersionList(CDBXSchemaPtr pSchema);
		static void RebuildIndex(const DBInfoPtr& p);
	};
}

#endif // __dbdiff_h__
