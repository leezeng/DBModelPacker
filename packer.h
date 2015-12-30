#ifndef __packer_h__
#define __packer_h__

#include "TextReader.h"

class CDBPatch;
class CDBFile;
class CDBModel;
class CDBVer;

typedef std::shared_ptr<CDBPatch> CDBPatchPtr;
typedef std::shared_ptr<CDBFile> CDBFilePtr;
typedef std::shared_ptr<CDBModel> CDBModelPtr;
typedef std::shared_ptr<CDBVer> CDBVerPtr;

class CDBPatch
{
public:
	INT32 m_version;
	std::list<std::string> m_sqlList;
};

class CDBFile
{
public:
	std::string m_name;
	std::list<CDBPatchPtr> m_patchList;
};

class CDBModel
{
public:
	int m_major;
	std::list<CDBFilePtr> m_fileList;
};

class CDBVer
{
public:
	std::list<CDBModelPtr> m_modelList;
};

class CRootPacker
{
public:
	CRootPacker();
	~CRootPacker();

	bool Run(const std::wstring& path, std::string& data);

	bool LoadModel(const std::wstring& path, const CDBModelPtr& p);
	bool LoadDB(const std::wstring& path, const CDBFilePtr& p);
	bool LoadPatch(const std::wstring& path, const CDBPatchPtr& p);

	void WriteInt32(INT32 value);
	void WriteString(const std::string& s);

	std::stringstream m_ss;
};

#endif // __packer_h__
