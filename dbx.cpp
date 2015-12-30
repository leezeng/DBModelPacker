#include "stdafx.h"
#include "dbx.h"
#include "DBModelTree.h"

class CDBXModelTreeHandlerImpl
	: public IDBModelTreeHandler
{
public:
	CDBXModelTreeHandlerImpl();

	bool Load(const wchar_t *pszRootDir, std::vector<CDBXSchemaPtr>& result);

	virtual bool BeginSchema(int majorVer);
	virtual bool BeginDatabase(const std::wstring& name);
	virtual bool HandlePatch(int ver, std::vector<std::string>& sqlList);
	virtual bool EndDatabase();
	virtual bool EndSchema();

private:
	std::vector<CDBXSchemaPtr> m_result;
};

CDBXModelTreeHandlerImpl::CDBXModelTreeHandlerImpl()
{
}

bool CDBXModelTreeHandlerImpl::Load(const wchar_t *pszRootDir, std::vector<CDBXSchemaPtr>& result)
{
	m_result.clear();

	if (!LoadDBModelTree(pszRootDir, this))
	{
		return false;
	}

	m_result.swap(result);
	return true;
}

bool CDBXModelTreeHandlerImpl::BeginSchema(int majorVer)
{
	auto p = std::make_shared<CDBXSchema>();
	p->majorVer = majorVer;
	m_result.push_back(p);
	return true;
}

bool CDBXModelTreeHandlerImpl::BeginDatabase(const std::wstring& name)
{
	auto p = std::make_shared<CDBXDatabase>();
	p->m_name = name;
	m_result.back()->m_dbList.push_back(p);
	return true;
}

bool CDBXModelTreeHandlerImpl::HandlePatch(int ver, std::vector<std::string>& sqlList)
{
	auto p = std::make_shared<CDBXPatch>();
	p->m_version = ver;
	p->m_sqlList.swap(sqlList);
	m_result.back()->m_dbList.back()->m_patchList.push_back(p);
	return true;
}

bool CDBXModelTreeHandlerImpl::EndDatabase()
{
	return true;
}

bool CDBXModelTreeHandlerImpl::EndSchema()
{
	return true;
}

bool DBXLoadAll(const wchar_t *pszRootDir, std::vector<CDBXSchemaPtr>& result)
{
	CDBXModelTreeHandlerImpl handler;
	return handler.Load(pszRootDir, result);
}
