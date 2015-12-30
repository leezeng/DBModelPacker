#ifndef __StringUtils_h__
#define __StringUtils_h__

#include <string>

class CStringUtils
{
public:
	static std::string ConvertWideToUtf8(const std::wstring& text);
	static std::string ConvertWideToUtf8(const wchar_t *pszText, int length = -1);
	static std::wstring ConvertUtf8ToWide(const std::string& text);
	static std::wstring ConvertUtf8ToWide(const char *pszText, int length = -1);
	static std::wstring Trim(const std::wstring& s);
};

#endif // __StringUtils_h__
