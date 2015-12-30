#ifndef __SQLiteWrapper_h__
#define __SQLiteWrapper_h__

#include "sqlite3.h"
#include <memory>
#include <string>

class CSQLWConnection;
class CSQLWStatement;

typedef std::shared_ptr<CSQLWConnection> CSQLWConnectionPtr;
typedef std::shared_ptr<CSQLWStatement> CSQLWStatementPtr;

class CSQLWConnection
{
public:
	CSQLWConnection();
	~CSQLWConnection();

	bool IsOpen() const;
	int Open(const wchar_t *pszFilename);
	int Close();

	int Prepare(const wchar_t *pszSql, int cch, const wchar_t **pszTail, CSQLWStatementPtr& pStatement);
	int Prepare(const char *pszSql, int cch, const char **pszTail, CSQLWStatementPtr& pStatement);
	int Execute(const wchar_t *pszSql, int cch, const wchar_t **pszTail);
	int Execute(const char *pszSql, int cch, const char **pszTail);

private:
	sqlite3 *m_conn;
};

class CSQLWStatement
{
public:
	CSQLWStatement(sqlite3_stmt *stmt);
	~CSQLWStatement();

	std::string GetSql();
	int Step();
	int GetColumnCount();
	int GetColumnType(int col);
	std::string GetColumnName(int col);
	std::string GetColumnText(int col);
	int Finalize();

private:
	sqlite3_stmt *m_stmt;
};

#endif // __SQLiteWrapper_h__
