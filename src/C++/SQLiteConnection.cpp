/* Copyright (c) 2026 - Daniel Morilha */

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#ifdef HAVE_SQLITE3
#include "SQLiteConnection.h"

namespace FIX {
/* SQLiteQuery */
SQLiteQuery::~SQLiteQuery() {
  sqlite3_finalize(m_statement);
  m_statement = nullptr;
}

SQLiteQuery::SQLiteQuery(const std::string &query) :
  m_statement(nullptr), m_query(query) { }

bool SQLiteQuery::execute(sqlite3 *pConnection) {
  /* does this return any error? */
  const int code = sqlite3_prepare_v2(pConnection, m_query.c_str(), m_query.size(), &m_statement, nullptr);
  if (SQLITE_OK == code) {
    return true;
  }
  m_status = sqlite3_errcode(pConnection);
  m_reason = sqlite3_errmsg(pConnection);

  return success();
}

bool SQLiteQuery::success() const {
  return m_status == 0;
}

int SQLiteQuery::rows() {
  if (m_rows.empty()) {
    populate();
  }
  return m_rows.size();
}

const std::string &SQLiteQuery::reason() const {
  return m_reason;
}

void SQLiteQuery::throwException() {
  if (!success()) {
    throw IOException("Query failed [" + m_query + "] " + reason());
  }
}

std::string SQLiteQuery::getValue(const int row, const int column) {
  if (m_rows.empty()) {
    populate();
  }
  return m_rows[row][column];
}

void SQLiteQuery::populate() {
  const int column_count = sqlite3_column_count(m_statement);
  int row = 0;
  while (SQLITE_ROW == sqlite3_step(m_statement)) {
    m_rows.emplace_back(Row{});
    for (int column = 0; column_count > column; ++column) {
      const char * const content = reinterpret_cast<const char *>(sqlite3_column_text(m_statement, column));
      m_rows.back().emplace_back(content);
    }
    ++row;
  }
}

/* SQLiteConnection */
SQLiteConnection::~SQLiteConnection() {
  sqlite3_close(m_pConnection);
  m_pConnection = nullptr;
}

SQLiteConnection::SQLiteConnection(const std::string &database) {
  sqlite3_open(database.c_str(), &m_pConnection);
}

bool SQLiteConnection::execute(SQLiteQuery &query) {
  /* should it lock ? */
  return query.execute(m_pConnection);
}

bool SQLiteConnection::execute(const char *statement) {
  /* should it lock ? */
  char *error = nullptr;
  const int result = sqlite3_exec(m_pConnection, statement, nullptr, nullptr, &error);
  if (nullptr != error) {
    sqlite3_free(error);
    error = nullptr;
  }
  return SQLITE_OK == result;
}
} // namespace FIX
#endif // HAVE_SQLITE3
