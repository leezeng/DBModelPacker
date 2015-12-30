#include "stdafx.h"
#include "SQLiteWrapper.h"
#include "StringUtils.h"

CSQLWConnection::CSQLWConnection()
	: m_conn(nullptr)
{
}

CSQLWConnection::~CSQLWConnection()
{
	Close();
}

bool CSQLWConnection::IsOpen() const
{
	return m_conn != nullptr;
}

int CSQLWConnection::Open(const wchar_t *pszFilename)
{
	if (m_conn != nullptr)
	{
		return SQLITE_ERROR;
	}

	auto s = CStringUtils::ConvertWideToUtf8(pszFilename);

	int r = sqlite3_open_v2(s.c_str(), &m_conn, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
	if (r != SQLITE_OK)
	{
		if (m_conn != nullptr)
		{
			sqlite3_close_v2(m_conn);
			m_conn = nullptr;
		}
	}

	return r;
}

int CSQLWConnection::Close()
{
	int r = SQLITE_OK;

	if (m_conn != nullptr)
	{
		r = sqlite3_close_v2(m_conn);
		m_conn = nullptr;
	}

	return r;
}

int CSQLWConnection::Prepare(const wchar_t *pszSql, int cch, const wchar_t **pszTail, CSQLWStatementPtr& pStatement)
{
	sqlite3_stmt *stmt = nullptr;
	int r = sqlite3_prepare16_v2(m_conn, pszSql, cch * 2, &stmt, reinterpret_cast<const void**>(pszTail));
	if (r == SQLITE_OK)
	{
		pStatement = std::make_shared<CSQLWStatement>(stmt);
	}

	return r;
}

int CSQLWConnection::Prepare(const char *pszSql, int cch, const char **pszTail, CSQLWStatementPtr& pStatement)
{
	sqlite3_stmt *stmt = nullptr;
	int r = sqlite3_prepare_v2(m_conn, pszSql, cch, &stmt, pszTail);
	if (r == SQLITE_OK)
	{
		pStatement = std::make_shared<CSQLWStatement>(stmt);
	}

	return r;
}

int CSQLWConnection::Execute(const wchar_t *pszSql, int cch, const wchar_t **pszTail)
{
	CSQLWStatementPtr stmt;
	int r = Prepare(pszSql, cch, pszTail, stmt);
	if (r == SQLITE_OK)
	{
		r = stmt->Step();
	}
	return r;
}

int CSQLWConnection::Execute(const char *pszSql, int cch, const char **pszTail)
{
	CSQLWStatementPtr stmt;
	int r = Prepare(pszSql, cch, pszTail, stmt);
	if (r == SQLITE_OK)
	{
		r = stmt->Step();
	}
	return r;
}

CSQLWStatement::CSQLWStatement(sqlite3_stmt *stmt)
	: m_stmt(stmt)
{
}

CSQLWStatement::~CSQLWStatement()
{
	Finalize();
}

std::string CSQLWStatement::GetSql()
{
	return sqlite3_sql(m_stmt);
}

int CSQLWStatement::Step()
{
	return sqlite3_step(m_stmt);
}

int CSQLWStatement::GetColumnCount()
{
	return sqlite3_column_count(m_stmt);
}

int CSQLWStatement::GetColumnType(int col)
{
	return sqlite3_column_type(m_stmt, col);
}

std::string CSQLWStatement::GetColumnName(int col)
{
	return sqlite3_column_name(m_stmt, col);
}

std::string CSQLWStatement::GetColumnText(int col)
{
	return reinterpret_cast<const char*>(sqlite3_column_text(m_stmt, col));
}

int CSQLWStatement::Finalize()
{
	int r = SQLITE_OK;

	if (m_stmt != nullptr)
	{
		r = sqlite3_finalize(m_stmt);
		m_stmt = nullptr;
	}

	return r;
}
