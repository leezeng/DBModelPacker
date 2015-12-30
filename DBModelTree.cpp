#include "stdafx.h"
#include "DBModelTree.h"
#include "TextReader.h"
#include "StringUtils.h"
#include <fstream>

bool LoadPatch(int ver, const std::wstring& path, IDBModelTreeHandler *pHandler)
{
	std::ifstream fIn(path, std::ios_base::in | std::ios_base::binary);
	if (!fIn)
	{
		return false;
	}

	CTextReader reader;
	if (!reader.Open(&fIn))
	{
		return false;
	}

	enum State
	{
		State_Normal,
		State_Start,
		State_InQuote,
	};

	State state = State_Normal;

	std::string s1;

	std::vector<std::string> sqlList;

	try
	{
		while (reader.HasMore())
		{
			char c = reader.GetChar();
			switch (state)
			{
			case State_Normal:
				if (!isspace(c))
				{
					reader.UngetChar(c);
					state = State_Start;
				}
				break;

			case State_Start:
				if (c == '"')
				{
					s1.push_back(c);
					state = State_InQuote;
				}
				else if (c == ';')
				{
					sqlList.push_back(s1);
					s1.clear();
					state = State_Normal;
				}
				else
				{
					s1.push_back(c);
				}
				break;

			case State_InQuote:
				if (c == '"')
				{
					s1.push_back(c);
					state = State_Start;
				}
				else
				{
					s1.push_back(c);
				}
			}
		}
	}
	catch (const CTextReader::Error&)
	{
		return false;
	}

	return pHandler->HandlePatch(ver, sqlList);
}

static int ParseVersion(const std::wstring& s)
{
	auto sep1 = s.find(L'.');
	if (sep1 == std::wstring::npos)
	{
		return -1;
	}

	auto sep2 = s.find(L'.', sep1 + 1);
	if (sep2 == std::wstring::npos)
	{
		return -2;
	}

	auto s1 = s.substr(0, sep1);
	auto s2 = s.substr(sep1 + 1, sep2 - sep1 - 1);
	auto s3 = s.substr(sep2 + 1);

	int major = _wtoi(s1.c_str());
	int minor = _wtoi(s2.c_str());
	int rev = _wtoi(s3.c_str());

	return major * 1000000 + minor * 1000 + rev;
}

static bool LoadDB(const std::wstring& name, const std::wstring& path, IDBModelTreeHandler *pHandler)
{
	std::wstring rootListPath(path);
	rootListPath += L"\\list.txt";

	std::ifstream fIn(rootListPath, std::ios_base::in | std::ios_base::binary);
	if (!fIn)
	{
		return false;
	}

	CTextReader reader;
	if (!reader.Open(&fIn))
	{
		return false;
	}

	try
	{
		while (reader.HasMore())
		{
			auto line = reader.ReadLine();
			if (!line.empty())
			{
				auto line1 = CStringUtils::ConvertUtf8ToWide(line);
				auto sep = line1.find(L',');
				if (sep != std::wstring::npos)
				{
					auto sVer = CStringUtils::Trim(line1.substr(0, sep));
					auto sDir = CStringUtils::Trim(line1.substr(sep + 1));

					int ver = ParseVersion(sVer);
					if (ver < 0)
					{
						return false;
					}

					if (!LoadPatch(ver, path + L"\\" + sDir, pHandler))
					{
						return false;
					}
				}
			}
		}
	}
	catch (const CTextReader::Error&)
	{
		return false;
	}

	return true;
}

static bool LoadModelVer(int major, const std::wstring& path, IDBModelTreeHandler *pHandler)
{
	std::wstring rootListPath(path);
	rootListPath += L"\\list.txt";

	std::ifstream fIn(rootListPath, std::ios_base::in | std::ios_base::binary);
	if (!fIn)
	{
		return false;
	}

	CTextReader reader;
	if (!reader.Open(&fIn))
	{
		return false;
	}

	try
	{
		while (reader.HasMore())
		{
			auto line = reader.ReadLine();
			if (!line.empty())
			{
				auto line1 = CStringUtils::ConvertUtf8ToWide(line);
				auto sep = line1.find(L',');
				if (sep != std::wstring::npos)
				{
					auto sType = CStringUtils::Trim(line1.substr(0, sep));
					auto sDir = CStringUtils::Trim(line1.substr(sep + 1));

					if (!pHandler->BeginDatabase(sType))
					{
						return false;
					}

					if (!LoadDB(sType, path + L"\\" + sDir, pHandler))
					{
						return false;
					}

					if (!pHandler->EndDatabase())
					{
						return false;
					}
				}
			}
		}
	}
	catch (const CTextReader::Error&)
	{
		return false;
	}

	return true;
}

bool LoadDBModelTree(const wchar_t *pszRoot, IDBModelTreeHandler *pHandler)
{
	std::wstring rootListPath(pszRoot);
	rootListPath += L"\\list.txt";

	std::ifstream fIn(rootListPath, std::ios_base::in | std::ios_base::binary);
	if (!fIn)
	{
		return false;
	}

	CTextReader reader;
	if (!reader.Open(&fIn))
	{
		return false;
	}

	try
	{
		while (reader.HasMore())
		{
			auto line = reader.ReadLine();
			if (!line.empty())
			{
				auto line1 = CStringUtils::ConvertUtf8ToWide(line);
				auto sep = line1.find(L',');
				if (sep != std::wstring::npos)
				{
					auto sMajor = CStringUtils::Trim(line1.substr(0, sep));
					auto sDir = CStringUtils::Trim(line1.substr(sep + 1));
					int major = _wtoi(sMajor.c_str());

					if (!pHandler->BeginSchema(major))
					{
						return false;
					}

					if (!LoadModelVer(major, std::wstring(pszRoot) + L"\\" + sDir, pHandler))
					{
						return false;
					}

					if (!pHandler->EndSchema())
					{
						return false;
					}
				}
			}
		}
	}
	catch (const CTextReader::Error&)
	{
		return false;
	}

	return true;
}
