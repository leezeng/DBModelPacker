#include "stdafx.h"
#include "geninc.h"
#include <stdio.h>
#include <algorithm>

static std::string GenerateCodeName(int& seq)
{
	char buff[200];
	sprintf_s(buff, "g_id%d", seq++);
	return buff;
}

void GenerateHeaderFile(std::vector<dbdiff::SchemaPtr>& vec, std::ostream& os)
{
	int seq = 10000;

	std::for_each(
		std::begin(vec),
		std::end(vec),
		[&](const dbdiff::SchemaPtr& pSchema)
	{
		pSchema->m_listCodeName = GenerateCodeName(seq);

		std::for_each(
			std::begin(pSchema->m_databaseRangeList),
			std::end(pSchema->m_databaseRangeList),
			[&](const dbdiff::DatabaseRangePtr& pDatabaseRange)
		{
			pDatabaseRange->m_codeName = GenerateCodeName(seq);
			pDatabaseRange->m_listCodeName = GenerateCodeName(seq);

			std::for_each(
				std::begin(pDatabaseRange->m_databaseList),
				std::end(pDatabaseRange->m_databaseList),
				[&](const dbdiff::DatabasePtr pDatabase)
			{
				pDatabase->m_listCodeName = GenerateCodeName(seq);

				std::for_each(
					std::begin(pDatabase->m_tableRangeList),
					std::end(pDatabase->m_tableRangeList),
					[&](const dbdiff::TableRangePtr& pTableRange)
				{
					pTableRange->m_codeName = GenerateCodeName(seq);
					pTableRange->m_listCodeName = GenerateCodeName(seq);

					std::for_each(
						std::begin(pTableRange->m_tableList),
						std::end(pTableRange->m_tableList),
						[&](const dbdiff::TablePtr& pTable)
					{
						pTable->m_listCodeName = GenerateCodeName(seq);

						std::for_each(
							std::begin(pTable->m_columnRangeList),
							std::end(pTable->m_columnRangeList),
							[&](const dbdiff::ColumnRangePtr& pColumnRange)
						{
							pColumnRange->m_codeName = GenerateCodeName(seq);
							pColumnRange->m_listCodeName = GenerateCodeName(seq);

							os << "static const DBColumn " << pColumnRange->m_listCodeName << "[] =" << std::endl;
							os << "{" << std::endl;

							std::for_each(
								std::begin(pColumnRange->m_columnList),
								std::end(pColumnRange->m_columnList),
								[&](const dbdiff::ColumnPtr& pColumn)
							{
								os << "\t{ \"" << pColumn->m_name << "\" }," << std::endl;
							});

							os << "};" << std::endl << std::endl;

							os << "static const DBColumnRange " << pColumnRange->m_codeName << " = { "
								<< pColumnRange->m_version << ", " << pColumnRange->m_columnList.size() << ", " << pColumnRange->m_listCodeName << " };" << std::endl << std::endl;
						});

						os << "static const DBColumnRange *" << pTable->m_listCodeName << "[] =" << std::endl;
						os << "{" << std::endl;

						std::for_each(
							std::begin(pTable->m_columnRangeList),
							std::end(pTable->m_columnRangeList),
							[&](const dbdiff::ColumnRangePtr& pColumnRange)
						{
							os << "\t{ &" << pColumnRange->m_codeName << " }," << std::endl;
						});

						os << "};" << std::endl << std::endl;

					});

					os << "static const DBTable " << pTableRange->m_listCodeName << "[] =" << std::endl;
					os << "{" << std::endl;

					std::for_each(
						std::begin(pTableRange->m_tableList),
						std::end(pTableRange->m_tableList),
						[&](const dbdiff::TablePtr& pTable)
					{
						os << "\t { \"" << pTable->m_name << "\", " << pTable->m_columnRangeList.size() << ", " << pTable->m_listCodeName << " }," << std::endl;
					});

					os << "};" << std::endl << std::endl;

					os << "static const DBTableRange " << pTableRange->m_codeName << " = { " << pTableRange->m_version << ", " << pTableRange->m_tableList.size() << ", " << pTableRange->m_listCodeName << " };" << std::endl << std::endl;
				});

				os << "static const DBTableRange *" << pDatabase->m_listCodeName << "[] =" << std::endl;
				os << "{" << std::endl;

				std::for_each(
					std::begin(pDatabase->m_tableRangeList),
					std::end(pDatabase->m_tableRangeList),
					[&](const dbdiff::TableRangePtr& pTableRange)
				{
					os << "\t{ &" << pTableRange->m_codeName << " }," << std::endl;
				});

				os << "};" << std::endl << std::endl;

			});

			os << "static const DBDatabase " << pDatabaseRange->m_listCodeName << "[] =" << std::endl;
			os << "{" << std::endl;

			std::for_each(
				std::begin(pDatabaseRange->m_databaseList),
				std::end(pDatabaseRange->m_databaseList),
				[&](const dbdiff::DatabasePtr& pDatabase)
			{
				os << "\t { \"" << pDatabase->m_name << "\", " << pDatabase->m_tableRangeList.size() << ", " << pDatabase->m_listCodeName << " }," << std::endl;
			});

			os << "};" << std::endl << std::endl;

			os << "static const DBDatabaseRange " << pDatabaseRange->m_codeName << " = { " << pDatabaseRange->m_version << ", " << pDatabaseRange->m_databaseList.size() << ", " << pDatabaseRange->m_listCodeName << " };" << std::endl << std::endl;

		});

		os << "static const DBDatabaseRange *" << pSchema->m_listCodeName << "[] =" << std::endl;
		os << "{" << std::endl;

		std::for_each(
			std::begin(pSchema->m_databaseRangeList),
			std::end(pSchema->m_databaseRangeList),
			[&](const dbdiff::DatabaseRangePtr& pDatabaseRange)
		{
			os << "\t{ &" << pDatabaseRange->m_codeName << " }," << std::endl;
		});

		os << "};" << std::endl << std::endl;
	});

	os << "static const DBSchema g_schemas[] =" << std::endl;
	os << "{" << std::endl;

	std::for_each(
		std::begin(vec),
		std::end(vec),
		[&](const dbdiff::SchemaPtr& pSchema)
	{
		os << "\t { " << pSchema->m_majorVer << ", " << pSchema->m_databaseRangeList.size() << ", " << pSchema->m_listCodeName << " }," << std::endl;
	});

	os << "};" << std::endl << std::endl;

/*
	std::for_each(
		std::begin(vec),
		std::end(vec),
		[&](const dbdiff::SchemaPtr& pSchema)
	{
		std::for_each(
			std::begin(pSchema->m_databaseRangeList),
			std::end(pSchema->m_databaseRangeList),
			[&](const dbdiff::DatabaseRangePtr& pDatabaseRange)
		{
			std::for_each(
				std::begin(pDatabaseRange->m_databaseList),
				std::end(pDatabaseRange->m_databaseList),
				[&](const dbdiff::DatabasePtr pDatabase)
			{
				std::for_each(
					std::begin(pDatabase->m_tableRangeList),
					std::end(pDatabase->m_tableRangeList),
					[&](const dbdiff::TableRangePtr& pTableRange)
				{
					std::for_each(
						std::begin(pTableRange->m_tableList),
						std::end(pTableRange->m_tableList),
						[&](const dbdiff::TablePtr& pTable)
					{
						std::for_each(
							std::begin(pTable->m_columnRangeList),
							std::end(pTable->m_columnRangeList),
							[&](const dbdiff::ColumnRangePtr& pColumnRange)
						{
							std::for_each(
								std::begin(pColumnRange->m_columnList),
								std::end(pColumnRange->m_columnList),
								[&](const dbdiff::ColumnPtr& pColumn)
							{
							});
						});
					});
				});
			});
		});
	});
*/
}
