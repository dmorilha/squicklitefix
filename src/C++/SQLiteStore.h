/* -*- C++ -*- */
/* Copyright (c) 2026 - Daniel Morilha */

#ifndef FIX_SQLITE_STORE_H
#define FIX_SQLITE_STORE_H

#ifndef HAVE_SQLITE3
#error SQLiteStore.h included, but HAVE_SQLITE3 not defined
#endif

#ifdef HAVE_SQLITE3

#include <fstream>
#include <string>
#include <vector>

#include "Exceptions.h"
#include "MessageStore.h"
#include "SQLiteConnection.h"
#include "SessionSettings.h"

namespace FIX {
/// Creates a SQLite based implementation of MessageStore.
class SQLiteStoreFactory : public MessageStoreFactory {
public:
  ~SQLiteStoreFactory() = default;
  MessageStore *create(const UtcTimeStamp &, const SessionID &);
  MessageStore *create(const UtcTimeStamp &, const SessionID &, const std::string &);
  void destroy(MessageStore *);
};

/// SQLite based implementation of MessageStore.
class SQLiteStore : public MessageStore {
public:
  ~SQLiteStore();
  SQLiteStore(const UtcTimeStamp &, const SessionID &, const std::string &, const bool create = true);

  bool set(SEQNUM, const std::string &) EXCEPT(IOException);
  void get(SEQNUM, SEQNUM, std::vector<std::string> &) const EXCEPT(IOException);

  SEQNUM getNextSenderMsgSeqNum() const EXCEPT(IOException) { return m_cache.getNextSenderMsgSeqNum(); }
  SEQNUM getNextTargetMsgSeqNum() const EXCEPT(IOException) { return m_cache.getNextTargetMsgSeqNum(); }

  void setNextSenderMsgSeqNum(SEQNUM) EXCEPT(IOException);
  void setNextTargetMsgSeqNum(SEQNUM) EXCEPT(IOException);
  void incrNextSenderMsgSeqNum() EXCEPT(IOException);
  void incrNextTargetMsgSeqNum() EXCEPT(IOException);

  UtcTimeStamp getCreationTime() const EXCEPT(IOException);

  void reset(const UtcTimeStamp &) EXCEPT(IOException);
  void refresh() EXCEPT(IOException);

private:
  void createTables();
  void populateCache();

  MemoryStore m_cache;
  SQLiteConnection *m_pConnection;
  SessionID m_sessionID;
};
} // namespace FIX

#endif // HAVE_SQLITE3
#endif // FIX_SQLITE_STORE_H
