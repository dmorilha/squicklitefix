/* -*- C++ -*- */
/* Copyright (c) 2026 - Daniel Morilha */

#ifndef FIX_SQLITE_LOG_H
#define FIX_SQLITE_LOG_H

#ifndef HAVE_SQLITE3
#error SQLite.h included, but HAVE_SQLITE3 not defined
#endif

#ifdef HAVE_SQLITE3

#include <fstream>
#include <string>

#include "Exceptions.h"
#include "Log.h"
#include "SQLiteConnection.h"
#include "SessionSettings.h"

namespace FIX {
/// SQLite based implementation of Log.
class SQLiteLog : public Log {
public:
  ~SQLiteLog();

  SQLiteLog(const std::string &, const bool createTable = true);
  SQLiteLog(const SessionID &, const std::string &, const bool createTable = true);

  void clear();
  void backup() { }

  void onIncoming(const std::string &value) { insert(m_incomingTable, value); }
  void onOutgoing(const std::string &value) { insert(m_outgoingTable, value); }
  void onEvent(const std::string &value) { insert(m_eventTable, value); }

  void setIncomingTable(const std::string &incomingTable) { m_incomingTable = incomingTable; }
  void setOutgoingTable(const std::string &outgoingTable) { m_outgoingTable = outgoingTable; }
  void setEventTable(const std::string &eventTable) { m_eventTable = eventTable; }

private:
  void createTable(const std::string &);

  void init(const bool);
  void insert(const std::string &, const std::string &);

  std::string m_incomingTable;
  std::string m_outgoingTable;
  std::string m_eventTable;
  SQLiteConnection *m_pConnection;
  SessionID *m_pSessionID;
};

/// Creates a SQLite based implementation of Log.
class SQLiteLogFactory : public LogFactory {
public:
  ~SQLiteLogFactory() = default;
  Log *create();
  Log *create(const SessionID &, const std::string &);
  void destroy(Log *);
};
} // namespace FIX

#endif // HAVE_SQLITE3
#endif // FIX_SQLITE_LOG_H
