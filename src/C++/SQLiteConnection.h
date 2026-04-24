/* -*- C++ -*- */
/* Copyright (c) 2026 - Daniel Morilha */

#ifndef FIX_SQLITE_CONNECTION_H
#define FIX_SQLITE_CONNECTION_H

#ifndef HAVE_SQLITE3
#error SQLiteConnection.h included, but HAVE_SQLITE3 not defined
#endif

#ifdef HAVE_SQLITE3

#include <sqlite3.h>
#include <string>
#include <vector>

#include "Exceptions.h"

namespace FIX {
class SQLiteConnection;

class SQLiteQuery {
public:
  ~SQLiteQuery();
  SQLiteQuery(const std::string &);
  bool execute(sqlite3 *);
  bool success() const;
  int rows();
  const std::string &reason() const;
  void throwException() EXCEPT(IOException);
  std::string getValue(const int, const int);

private:
  using Row = std::vector<std::string>;
  void populate();

  sqlite3_stmt *m_statement;
  int m_status;
  const std::string m_query;
  std::string m_reason;
  std::vector<Row> m_rows;
};

class SQLiteConnection {
public:
  ~SQLiteConnection();
  SQLiteConnection(const std::string &);
  bool execute(SQLiteQuery &);
  bool execute(const char *);

private:
  sqlite3 *m_pConnection;
};
} // namespace FIX

#endif // HAVE_SQLITE3
#endif // FIX_SQLITE_CONNECTION_H
