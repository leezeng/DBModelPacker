#ifndef __dbx_h__
#define __dbx_h__

#include <memory>
#include <string>
#include <vector>

class CDBXSchema;
class CDBXDatabase;
class CDBXPatch;

typedef std::shared_ptr<CDBXSchema> CDBXSchemaPtr;
typedef std::shared_ptr<CDBXDatabase> CDBXDatabasePtr;
typedef std::shared_ptr<CDBXPatch> CDBXPatchPtr;

class CDBXSchema
{
public:
	int majorVer;
	std::vector<CDBXDatabasePtr> m_dbList;
};

class CDBXDatabase
{
public:
	std::wstring m_name;
	std::vector<CDBXPatchPtr> m_patchList;
};

class CDBXPatch
{
public:
	int m_version;
	std::vector<std::string> m_sqlList;
};

bool DBXLoadAll(const wchar_t *pszRootDir, std::vector<CDBXSchemaPtr>& result);

#endif // __test1_h__
