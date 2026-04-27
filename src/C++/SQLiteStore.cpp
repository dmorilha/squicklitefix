/* Copyright (c) 2026 - Daniel Morilha */

#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#ifdef HAVE_SQLITE3
#include "SQLiteStore.h"

namespace FIX {

/* SQLiteStoreFactory */
MessageStore *SQLiteStoreFactory::create(const UtcTimeStamp &now, const SessionID &sessionID) {
  const std::string database = "quickfix.db";
  return create(now, sessionID, database);
}

MessageStore *SQLiteStoreFactory::create(const UtcTimeStamp &now, const SessionID &sessionID, const std::string &database) {
  return new SQLiteStore(now, sessionID, database, true);
}

void SQLiteStoreFactory::destroy(MessageStore *pStore) {
  delete pStore;
}

/* SQLiteStore */
SQLiteStore::~SQLiteStore() {
  delete m_pConnection;
  m_pConnection = nullptr;
}

SQLiteStore::SQLiteStore(const UtcTimeStamp &now, const SessionID &sessionID, const std::string &database, const bool create)
  : m_cache(now), m_sessionID(sessionID) {
  m_pConnection = new SQLiteConnection(database);
  if (create) {
    createTables();
  }
  populateCache();
}

bool SQLiteStore::set(SEQNUM msgSeqNum, const std::string &msg) EXCEPT(IOException) {
  const long long sequenceNumber = msgSeqNum ^ 0x8000000000000000;
  std::string error;
  auto statement = SQLiteStatement::create("INSERT INTO messages (beginstring, sendercompid, targetcompid, session_qualifier, msgseqnum, message) VALUES ('%q', '%q', '%q', '%q', %lld, '%q');",
      m_sessionID.getBeginString().getValue().c_str(),
      m_sessionID.getSenderCompID().getValue().c_str(),
      m_sessionID.getTargetCompID().getValue().c_str(),
      m_sessionID.getSessionQualifier().c_str(),
      sequenceNumber, msg.c_str());

  if (!m_pConnection->execute(statement, error)) {
    auto statement = SQLiteStatement::create("UPDATE messages SET message='%q' WHERE beginstring='%q' AND snedercompid='%q' AND targetcompid='%q' AND session_qualifier='%q' AND msgseqnum=%lld;",
        msg.c_str(),
        m_sessionID.getBeginString().getValue().c_str(),
        m_sessionID.getSenderCompID().getValue().c_str(),
        m_sessionID.getTargetCompID().getValue().c_str(),
        m_sessionID.getSessionQualifier().c_str(),
        sequenceNumber);
    if (!m_pConnection->execute(statement, error)) {
      throw IOException("Insertion and update failed " + error);
    }
  }
  return true;
}

void SQLiteStore::get(SEQNUM begin, SEQNUM end, std::vector<std::string> &result) const EXCEPT(IOException) {
  const long begin_ = begin ^ 0x8000000000000000,
        end_ = end ^ 0x8000000000000000;
  result.clear();
  auto queryString = SQLiteStatement::create("SELECT message FROM messages WHERE beginstring='%q' AND sendercompid='%q' AND targetcompid='%q' AND session_qualifier='%q' AND msgseqnum>=%lld AND msgseqnum<=%lld ORDER BY msgseqnum;",
      m_sessionID.getBeginString().getValue().c_str(),
      m_sessionID.getSenderCompID().getValue().c_str(),
      m_sessionID.getTargetCompID().getValue().c_str(),
      m_sessionID.getSessionQualifier().c_str(),
      begin_, end_);
  SQLiteQuery query(queryString);
  if (!m_pConnection->execute(query)) {
    query.throwException();
  }
  int rows = query.rows();
  for (int row = 0; row < rows; row++) {
    result.push_back(query.getValue(row, 0));
  }
}

void SQLiteStore::setNextSenderMsgSeqNum(SEQNUM value) EXCEPT(IOException) {
  const long value_ = value ^ 0x8000000000000000;
  auto queryString = SQLiteStatement::create("UPDATE sessions SET outgoing_seqnum=%lld WHERE beginstring='%q' AND sendercompid='%q' AND targetcompid='%q' AND session_qualifier='%q';",
    value_,
    m_sessionID.getBeginString().getValue().c_str(),
    m_sessionID.getSenderCompID().getValue().c_str(),
    m_sessionID.getTargetCompID().getValue().c_str(),
    m_sessionID.getSessionQualifier().c_str());
  std::string error;
  if (!m_pConnection->execute(queryString, error)) {
    throw IOException("Update failed " + error);
  }
  m_cache.setNextSenderMsgSeqNum(value);
}

void SQLiteStore::setNextTargetMsgSeqNum(SEQNUM value) EXCEPT(IOException) {
  const long value_ = value ^ 0x8000000000000000;
  auto queryString = SQLiteStatement::create("UPDATE sessions SET incoming_seqnum=%lld WHERE beginstring='%q' AND sendercompid='%q' AND targetcompid='%q' AND session_qualifier='%q';",
    value_,
    m_sessionID.getBeginString().getValue().c_str(),
    m_sessionID.getSenderCompID().getValue().c_str(),
    m_sessionID.getTargetCompID().getValue().c_str(),
    m_sessionID.getSessionQualifier().c_str());
  std::string error;
  if (!m_pConnection->execute(queryString, error)) {
    throw IOException("Update failed " + error);
  }
  m_cache.setNextTargetMsgSeqNum(value);
}

void SQLiteStore::incrNextSenderMsgSeqNum() EXCEPT(IOException) {
  m_cache.incrNextSenderMsgSeqNum();
  setNextSenderMsgSeqNum(m_cache.getNextSenderMsgSeqNum());
}

void SQLiteStore::incrNextTargetMsgSeqNum() EXCEPT(IOException) {
  m_cache.incrNextTargetMsgSeqNum();
  setNextTargetMsgSeqNum(m_cache.getNextTargetMsgSeqNum());
}

UtcTimeStamp SQLiteStore::getCreationTime() const EXCEPT(IOException) { return m_cache.getCreationTime(); }

void SQLiteStore::reset(const UtcTimeStamp &now) EXCEPT(IOException) {
  auto queryString = SQLiteStatement::create("DELETE FROM messages WHERE beginstring='%q' AND sendercompid='%q' AND targetcompid='%q' AND session_qualifier='%q';", 
      m_sessionID.getBeginString().getValue().c_str(),
      m_sessionID.getSenderCompID().getValue().c_str(),
      m_sessionID.getTargetCompID().getValue().c_str(),
      m_sessionID.getSessionQualifier().c_str());
  std::string error;
  if (!m_pConnection->execute(queryString, error)) {
    throw IOException("Delete failed " + error);
  }
  m_cache.reset(now);
  UtcTimeStamp time = m_cache.getCreationTime();

  int year, month, day, hour, minute, second, millis;
  time.getYMD(year, month, day);
  time.getHMS(hour, minute, second, millis);

  char sqlTime[100];
  STRING_SPRINTF(sqlTime, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
  const long incoming_seqnum = m_cache.getNextTargetMsgSeqNum() ^ 0x8000000000000000,
    outgoing_seqnum = m_cache.getNextSenderMsgSeqNum() ^ 0x8000000000000000;

  auto queryString2 = SQLiteStatement::create("UPDATE sessions SET creation_time='%q', incoming_seqnum=%lld, outgoing_seqnum=%lld WHERE beginstring='%q' AND sendercompid='%q' AND targetcompid='%q' AND session_qualifier='%q';",
      sqlTime,
      incoming_seqnum,
      outgoing_seqnum,
      m_sessionID.getBeginString().getValue().c_str(),
      m_sessionID.getSenderCompID().getValue().c_str(),
      m_sessionID.getTargetCompID().getValue().c_str(),
      m_sessionID.getSessionQualifier().c_str());
  if (!m_pConnection->execute(queryString2, error)) {
    throw IOException("Update failed " + error);
  }
}

void SQLiteStore::refresh() EXCEPT(IOException) {
  m_cache.reset(UtcTimeStamp::now());
  populateCache();
}

void SQLiteStore::createTables() {
  {
    const char * const queryString =
      "CREATE TABLE IF NOT EXISTS sessions ("
        "beginstring TEXT NOT NULL,"
        "sendercompid TEXT NOT NULL,"
        "targetcompid TEXT NOT NULL,"
        "session_qualifier TEXT NOT NULL,"
        "creation_time NUMERIC NOT NULL,"
        "incoming_seqnum INTEGER NOT NULL, "
        "outgoing_seqnum INTEGER NOT NULL,"
        "PRIMARY KEY (beginstring, sendercompid, targetcompid, session_qualifier)"
      ");";
    std::string error;
    if (!m_pConnection->execute(queryString, error)) {
      throw IOException("Table creation failed " + error);
    }
  }
  {
    const char * const queryString = 
      "CREATE TABLE IF NOT EXISTS messages ("
        "beginstring TEXT NOT NULL,"
        "sendercompid TEXT NOT NULL,"
        "targetcompid TEXT NOT NULL,"
        "session_qualifier TEXT NOT NULL,"
        "msgseqnum INTEGER NOT NULL, "
        "message BLOB NOT NULL,"
        "PRIMARY KEY (beginstring, sendercompid, targetcompid, session_qualifier, msgseqnum)"
      ");";
    std::string error;
    if (!m_pConnection->execute(queryString, error)) {
      throw IOException("Table creation failed " + error);
    }
  }
}

void SQLiteStore::populateCache() {
  auto queryString = SQLiteStatement::create("SELECT creation_time, incoming_seqnum, outgoing_seqnum FROM sessions WHERE beginstring='%q' AND sendercompid='%q' AND targetcompid='%q' AND session_qualifier='%q';",
      m_sessionID.getBeginString().getValue().c_str(),
      m_sessionID.getSenderCompID().getValue().c_str(),
      m_sessionID.getTargetCompID().getValue().c_str(),
      m_sessionID.getSessionQualifier().c_str());
  SQLiteQuery query(queryString);
  if (!m_pConnection->execute(query)) {
    throw ConfigError("Unable to query sessions table");
  }

  int rows = query.rows();
  if (rows > 1) {
    throw ConfigError("Multiple entries found for session in database");
  }

  if (rows == 1) {
    struct tm time;
    std::string sqlTime = query.getValue(0, 0);
    strptime(sqlTime.c_str(), "%Y-%m-%d %H:%M:%S", &time);
    m_cache.setCreationTime(UtcTimeStamp(&time));
    const long incoming_seqnum = std::stoll(query.getValue(0, 1)),
      outgoing_seqnum = std::stoll(query.getValue(0, 2));
    const unsigned long target = incoming_seqnum ^ 0x8000000000000000,
      sender = outgoing_seqnum ^ 0x8000000000000000;
    m_cache.setNextTargetMsgSeqNum(target);
    m_cache.setNextSenderMsgSeqNum(sender);
  } else {
    UtcTimeStamp time = m_cache.getCreationTime();
    char sqlTime[100];
    int year, month, day, hour, minute, second, millis;
    time.getYMD(year, month, day);
    time.getHMS(hour, minute, second, millis);
    STRING_SPRINTF(sqlTime, "%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    const long incoming_seqnum = m_cache.getNextTargetMsgSeqNum() ^ 0x8000000000000000,
      outgoing_seqnum = m_cache.getNextTargetMsgSeqNum() ^ 0x8000000000000000;
    auto queryString2 = SQLiteStatement::create("INSERT INTO sessions (beginstring, sendercompid, targetcompid, session_qualifier, creation_time, incoming_seqnum, outgoing_seqnum) VALUES ('%q', '%q', '%q', '%q', '%q', %lld, %lld);",
        m_sessionID.getBeginString().getValue().c_str(),
        m_sessionID.getSenderCompID().getValue().c_str(),
        m_sessionID.getTargetCompID().getValue().c_str(),
        m_sessionID.getSessionQualifier().c_str(),
        sqlTime,
        incoming_seqnum,
        outgoing_seqnum);
    std::string error;
    if (!m_pConnection->execute(queryString2, error)) {
      throw ConfigError("Unable to create session in database " + error);
    }
  }
}

} // namespace FIX
#endif // HAVE_SQLITE3
