#ifndef __DBModelTree_h__
#define __DBModelTree_h__

#include <string>
#include <vector>

class IDBModelTreeHandler
{
public:
	virtual bool BeginSchema(int majorVer) = 0;
	virtual bool BeginDatabase(const std::wstring& name) = 0;
	virtual bool HandlePatch(int ver, std::vector<std::string>& sqlList) = 0;
	virtual bool EndDatabase() = 0;
	virtual bool EndSchema() = 0;
};

bool LoadDBModelTree(const wchar_t *pszRoot, IDBModelTreeHandler *pHandler);

#endif // __DBModelTree_h__
