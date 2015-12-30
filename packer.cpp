#include "stdafx.h"
#include "packer.h"

std::wstring ConvertUTF8ToWide(const std::string& utf8String)
{
	std::wstring wideString;

	if (utf8String.empty())
	{
		return wideString;
	}

	std::vector<wchar_t> vec1((utf8String.length() * 4) + 1);

	int len = MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), utf8String.length(),
		vec1.data(), vec1.size());

	if (len > 0)
	{
		wideString.assign(vec1.data(), vec1.data() + len);
	}

	return wideString;
}

std::string ConvertWideToUTF8(const std::wstring& wideString)
{
	std::string utf8String;

	if (wideString.empty())
	{
		return utf8String;
	}

	std::vector<char> vec1(wideString.length() * 6 + 1);

	int len = WideCharToMultiByte(CP_UTF8, 0, wideString.c_str(), wideString.length(),
		vec1.data(), vec1.size(), NULL, NULL);

	if (len > 0)
	{
		utf8String.assign(vec1.data(), vec1.data() + len);
	}

	return utf8String;
}

std::wstring Trim(const std::wstring& s)
{
	std::wstring result;

	enum State
	{
		State_Begin,
		State_Copy,
		State_Guess,
		State_End,
	};

	State state = State_Begin;

	std::wstring guess;

	for (size_t i = 0; i != s.length(); i++)
	{
		auto c = s.at(i);

		switch (state)
		{
		case State_Begin:
			if (!iswspace(c))
			{
				result.push_back(c);
				state = State_Copy;
			}
			break;

		case State_Copy:
			if (!iswspace(c))
			{
				result.push_back(c);
			}
			else
			{
				guess.push_back(c);
				state = State_Guess;
			}
			break;

		case State_Guess:
			if (iswspace(c))
			{
				guess.push_back(c);
			}
			else
			{
				result += guess;
				guess.clear();
				result.push_back(c);
				state = State_Copy;
			}
			break;
		}
	}

	return result;
}

int ParseVersion(const std::wstring& s)
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

CRootPacker::CRootPacker()
{
}

CRootPacker::~CRootPacker()
{
}

bool CRootPacker::Run(const std::wstring& path, std::string& data)
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

	auto pDBVer = std::make_shared<CDBVer>();

	try
	{
		while (reader.HasMore())
		{
			auto line = reader.ReadLine();
			if (!line.empty())
			{
				auto line1 = ConvertUTF8ToWide(line);
				auto sep = line1.find(L',');
				if (sep != std::wstring::npos)
				{
					auto sMajor = Trim(line1.substr(0, sep));
					auto sDir = Trim(line1.substr(sep + 1));
					int major = _wtoi(sMajor.c_str());

					auto pDBModel = std::make_shared<CDBModel>();
					pDBModel->m_major = major;

					if (LoadModel(path + L"\\" + sDir, pDBModel))
					{
						pDBVer->m_modelList.push_back(pDBModel);
					}
					else
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

	WriteInt32(pDBVer->m_modelList.size());
	std::for_each(pDBVer->m_modelList.begin(), pDBVer->m_modelList.end(),
		[=](const CDBModelPtr& pModel)
	{
		WriteInt32(pModel->m_major);
		WriteInt32(pModel->m_fileList.size());

		std::for_each(pModel->m_fileList.begin(), pModel->m_fileList.end(),
			[=](const CDBFilePtr& pFile)
		{
			WriteString(pFile->m_name);
			WriteInt32(pFile->m_patchList.size());

			std::for_each(pFile->m_patchList.begin(), pFile->m_patchList.end(),
				[=](const CDBPatchPtr& pPatch)
			{
				WriteInt32(pPatch->m_version);
				WriteInt32(pPatch->m_sqlList.size());

				std::for_each(pPatch->m_sqlList.begin(), pPatch->m_sqlList.end(),
					[=](const std::string& s)
				{
					WriteString(s);
				});
			});
		});
	});

	auto s = m_ss.str();
	data.swap(s);
	return true;
}

bool CRootPacker::LoadModel(const std::wstring& path, const CDBModelPtr& p)
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
				auto line1 = ConvertUTF8ToWide(line);
				auto sep = line1.find(L',');
				if (sep != std::wstring::npos)
				{
					auto sType = Trim(line1.substr(0, sep));
					auto sDir = Trim(line1.substr(sep + 1));

					auto pDBFile = std::make_shared<CDBFile>();
					pDBFile->m_name = ConvertWideToUTF8(sType);

					if (LoadDB(path + L"\\" + sDir, pDBFile))
					{
						p->m_fileList.push_back(pDBFile);
					}
					else
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

bool CRootPacker::LoadDB(const std::wstring& path, const CDBFilePtr& p)
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
				auto line1 = ConvertUTF8ToWide(line);
				auto sep = line1.find(L',');
				if (sep != std::wstring::npos)
				{
					auto sVer = Trim(line1.substr(0, sep));
					auto sDir = Trim(line1.substr(sep + 1));

					int ver = ParseVersion(sVer);
					if (ver < 0)
					{
						return false;
					}

					auto pPatch = std::make_shared<CDBPatch>();
					pPatch->m_version = ver;

					if (LoadPatch(path + L"\\" + sDir, pPatch))
					{
						p->m_patchList.push_back(pPatch);
					}
					else
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

bool CRootPacker::LoadPatch(const std::wstring& path, const CDBPatchPtr& p)
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
					OutputDebugStringA(s1.c_str());
					OutputDebugStringA("\n");
					p->m_sqlList.push_back(s1);
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

	return true;
}

void CRootPacker::WriteInt32(INT32 value)
{
	m_ss.write(reinterpret_cast<char*>(&value), sizeof(value));
}

void CRootPacker::WriteString(const std::string& s)
{
	WriteInt32(s.length());
	m_ss.write(s.c_str(), s.length());
}
