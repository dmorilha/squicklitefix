/* -*- C++ -*- */
/* Copyright (c) 2026 - Daniel Morilha */

#ifndef FIX_SQLITE_CONNECTION_H
#define FIX_SQLITE_CONNECTION_H

#ifndef HAVE_SQLITE3
#error SQLiteConnection.h included, but HAVE_SQLITE3 not defined
#endif

#ifdef HAVE_SQLITE3

#include <shared_mutex>
#include <string>
#include <vector>

#include <sqlite3.h>

#include "Exceptions.h"

namespace FIX {
class SQLiteConnection;

class SQLiteStatement {
public:
  template<class ... Args>
  static SQLiteStatement create(Args && ... args) {
    SQLiteStatement result;
    result.m_Statement = sqlite3_mprintf(std::forward<Args>(args)...);
    return result;
  }
  ~SQLiteStatement();
  operator char * () const { return m_Statement; }

private:
  SQLiteStatement() = default;
  char *m_Statement = nullptr;
};

class SQLiteQuery {
public:
  ~SQLiteQuery();
  SQLiteQuery(const std::string &);
  SQLiteQuery(const char *);
  bool execute(sqlite3 *);
  bool success() const;
  int rows();
  const std::string &reason() const;
  void throwException() EXCEPT(IOException);
  std::string getValue(const int, const int);

private:
  using Row = std::vector<std::string>;
  void populate();

  sqlite3_stmt *m_statement = nullptr;
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
  bool execute(const char *, std::string &);

private:
  sqlite3 *m_pConnection = nullptr;
  mutable std::shared_mutex m_pMutex;
};
} // namespace FIX

#endif // HAVE_SQLITE3
#endif // FIX_SQLITE_CONNECTION_H
