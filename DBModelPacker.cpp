// DBModelPacker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "packer.h"
#include "dbx.h"
#include "geninc.h"
#include <fstream>

bool GenerateDictSource(const wchar_t *pszRootDir, const wchar_t *pszOutputPath);

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 4)
	{
		return -1;
	}

	CRootPacker packer;
	std::string data;
	if (!packer.Run(argv[1], data))
	{
		return 1;
	}

	FILE *fp;
	if (0 != _wfopen_s(&fp, argv[2], L"wb"))
	{
		return 2;
	}

	fwrite("\xef\xbb\xbf", 1, 3, fp);
	fprintf(fp, "%s", "static const unsigned char g_data[] =\r\n{\r\n");

	size_t ww = 16;

	for (size_t i = 0; i < data.length(); i += ww)
	{
		auto rem = data.length() - i;
		rem = rem < ww ? rem : ww;

		fprintf(fp, "\t");

		for (size_t j = 0; j < rem; j++)
		{
			fprintf(fp, "'\\x%02x', ", static_cast<unsigned char>(data[i + j]));
		}

		fprintf(fp, "\r\n");
	}


	fprintf(fp, "%s", "};\r\n");

	fclose(fp);

	if (!GenerateDictSource(argv[1], argv[4]))
	{
		return 3;
	}

	return 0;
}

bool GenerateDictSource(const wchar_t *pszRootDir, const wchar_t *pszOutputPath)
{
	std::vector<CDBXSchemaPtr> result;

	if (!DBXLoadAll(pszRootDir, result))
	{
		return false;
	}

	try
	{
		std::vector<dbdiff::SchemaPtr> vec1;

		for (auto i = 0; i != result.size(); i++)
		{
			auto pSchema = result.at(i);

			dbdiff::SchemaPtr pResult;
			if (!dbdiff::CDiffEngine::Process(pSchema, pResult))
			{
				return false;
			}
			vec1.push_back(pResult);
		}

		std::ofstream out1(pszOutputPath, std::ios_base::out);

		GenerateHeaderFile(vec1, out1);
	}
	catch (...)
	{
		return false;
	}

	return true;
}
