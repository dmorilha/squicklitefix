/* Copyright (c) 2026 - Daniel Morilha */

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#ifdef HAVE_SQLITE3
#include "SQLiteLog.h"

namespace FIX {
/* SQLiteLog */
SQLiteLog::~SQLiteLog() {
  delete m_pConnection;
  m_pConnection = nullptr;
}

SQLiteLog::SQLiteLog(const std::string &database, const bool create) : m_pSessionID(nullptr) {
  m_pConnection = new SQLiteConnection(database);
  init(create);
}

SQLiteLog::SQLiteLog(const SessionID &sessionID, const std::string &database, const bool create) {
  m_pSessionID = new SessionID(sessionID);
  m_pConnection = new SQLiteConnection(database);
  init(create);
}

void SQLiteLog::clear() {
  std::stringstream whereClause;

  whereClause << "WHERE ";

  if (m_pSessionID) {
    whereClause << "BeginString = '" << m_pSessionID->getBeginString().getValue() << "' "
                << "AND SenderCompID = '" << m_pSessionID->getSenderCompID().getValue() << "' "
                << "AND TargetCompID = '" << m_pSessionID->getTargetCompID().getValue() << "'";

    if (m_pSessionID->getSessionQualifier().size()) {
      whereClause << "AND SessionQualifier = '" << m_pSessionID->getSessionQualifier() << "'";
    }
  } else {
    whereClause << "BeginString = NULL AND SenderCompID = NULL AND TargetCompID = NULL";
  }

  {
    std::stringstream incomingQuery;
    incomingQuery << "DELETE FROM " << m_incomingTable << " " << whereClause.str() << ";";
    SQLiteQuery incoming(incomingQuery.str());
    m_pConnection->execute(incoming);
  }

  {
    std::stringstream outgoingQuery;
    outgoingQuery << "DELETE FROM " << m_outgoingTable << " " << whereClause.str() << ";";
    SQLiteQuery outgoing(outgoingQuery.str());
    m_pConnection->execute(outgoing);
  }

  {
    std::stringstream eventQuery;
    eventQuery << "DELETE FROM " << m_eventTable << " " << whereClause.str() << ";";
    SQLiteQuery event(eventQuery.str());
    m_pConnection->execute(event);
  }
}

void SQLiteLog::createTable(const std::string &tableName) {
  std::stringstream queryString;
  queryString << "CREATE TABLE IF NOT EXISTS " << tableName << " ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "time NUMERIC NOT NULL,"
    "time_milliseconds INT NOT NULL,"
    "beginstring TEXT,"
    "sendercompid TEXT,"
    "targetcompid TEXT,"
    "session_qualifier TEXT,"
    "text BLOB NOT NULL"
    ");";
  if (!m_pConnection->execute(queryString.str().c_str())) {
    /* error */
  }
}

void SQLiteLog::init(const bool create) {
  setIncomingTable("messages_log");
  setOutgoingTable("messages_log");
  setEventTable("event_log");
  if (create) {
    createTable(m_incomingTable);
    createTable(m_outgoingTable);
    createTable(m_eventTable);
  }
}

void SQLiteLog::insert(const std::string &table, const std::string &value) {
  UtcTimeStamp time = UtcTimeStamp::now();
  int year, month, day, hour, minute, second, millis;
  time.getYMD(year, month, day);
  time.getHMS(hour, minute, second, millis);

  char sqlTime[100];
  STRING_SPRINTF(sqlTime, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

  std::stringstream queryString;
  queryString << "INSERT INTO " << table << " "
              << "(time, time_milliseconds, beginstring, sendercompid, targetcompid, session_qualifier, text) "
              << "VALUES ("
              << "'" << sqlTime << "','" << millis << "',";

  if (m_pSessionID) {
    queryString << "'" << m_pSessionID->getBeginString().getValue() << "',"
                << "'" << m_pSessionID->getSenderCompID().getValue() << "',"
                << "'" << m_pSessionID->getTargetCompID().getValue() << "',";
    if (m_pSessionID->getSessionQualifier() == "") {
      queryString << "NULL" << ",";
    } else {
      queryString << "'" << m_pSessionID->getSessionQualifier() << "',";
    }
  } else {
    queryString << "NULL, NULL, NULL, NULL, ";
  }

  char * const valueCopy = sqlite3_mprintf("'%q');", value.c_str());
  queryString << valueCopy;
  sqlite3_free(reinterpret_cast<void*>(valueCopy));

  SQLiteQuery query(queryString.str());
  m_pConnection->execute(query);
}

/* SQLiteLogFactory */
Log *SQLiteLogFactory::create() {
  static const std::string database = "quickfix.db";
  return new SQLiteLog(database);
}

Log *SQLiteLogFactory::create(const SessionID &sessionId, const std::string &database) {
  return new SQLiteLog(sessionId, database);
}

void SQLiteLogFactory::destroy(Log *pLog) { delete pLog; }
} // namespace FIX

#endif // HAVE_SQLITE3
