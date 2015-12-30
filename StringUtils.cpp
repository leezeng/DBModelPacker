#include "stdafx.h"
#include "StringUtils.h"
#include <windows.h>
#include <vector>

static int MultiByteToWideChar(
	UINT CodePage,
	DWORD dwFlags,
	LPCSTR lpMultiByteStr,
	int cbMultiByte,
	std::wstring& wideString)
{
	WCHAR buff[512];

	int len = ::MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, buff, ARRAYSIZE(buff));
	if (len > 0)
	{
		wideString.assign(buff, buff + len);
		return len;
	}

	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		len = ::MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, NULL, 0);
		if (len > 0)
		{
			std::vector<WCHAR> buff1(len);
			len = ::MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, buff1.data(), len);
			if (len > 0)
			{
				wideString.assign(buff1.data(), buff1.data() + len);
				return len;
			}
		}
	}

	return 0;
}

static int WideCharToMultiByte(
	UINT CodePage,
	DWORD dwFlags,
	LPCWSTR lpWideCharStr,
	int cchWideChar,
	std::string& multiByteString,
	LPCSTR lpDefaultChar = NULL,
	LPBOOL lpUsedDefaultChar = NULL)
{
	char buff[512];

	int len = ::WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, buff, ARRAYSIZE(buff), lpDefaultChar, lpUsedDefaultChar);
	if (len > 0)
	{
		multiByteString.assign(buff, buff + len);
		return len;
	}

	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		len = ::WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, NULL, 0, lpDefaultChar, lpUsedDefaultChar);
		if (len > 0)
		{
			std::vector<char> buff1(len);
			len = ::WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, buff1.data(), len, lpDefaultChar, lpUsedDefaultChar);
			if (len > 0)
			{
				multiByteString.assign(buff1.data(), buff1.data() + len);
				return len;
			}
		}
	}

	return 0;
}

std::string CStringUtils::ConvertWideToUtf8(const std::wstring& text)
{
	return ConvertWideToUtf8(text.c_str(), static_cast<int>(text.length()));
}

std::string CStringUtils::ConvertWideToUtf8(const wchar_t *pszText, int length)
{
	std::string s;
	WideCharToMultiByte(CP_UTF8, 0, pszText, length, s);
	return s;
}

std::wstring CStringUtils::ConvertUtf8ToWide(const std::string& text)
{
	return ConvertUtf8ToWide(text.c_str(), static_cast<int>(text.length()));
}

std::wstring CStringUtils::ConvertUtf8ToWide(const char *pszText, int length)
{
	std::wstring s;
	MultiByteToWideChar(CP_UTF8, 0, pszText, length, s);
	return s;
}

std::wstring CStringUtils::Trim(const std::wstring& s)
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
