#ifdef _MSC_VER
#include "stdafx.h"
#else
#include "config.h"
#endif

#ifdef HAVE_SQLITE3

#include "MessageStoreTestCase.h"
#include "TestHelper.h"
#include <SQLiteStore.h>

#include "catch_amalgamated.hpp"

using namespace FIX;

struct SQLiteStoreFixture {
  SQLiteStoreFixture(bool reset) {
    SessionID sessionID(BeginString("FIX.4.2"), SenderCompID("SETGET"), TargetCompID("TEST"));

    try {
      object = factory.create(UtcTimeStamp::now(), sessionID);
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
      throw;
    }

    if (reset) {
      object->reset(UtcTimeStamp::now());
    }

    this->resetAfter = reset;
  }

  ~SQLiteStoreFixture() { factory.destroy(object); }

  SQLiteStoreFactory factory;
  MessageStore *object;
  bool resetAfter;
};

struct noResetSQLiteStoreFixture : SQLiteStoreFixture {
  noResetSQLiteStoreFixture()
      : SQLiteStoreFixture(false) {}
};

struct resetSQLiteStoreFixture : SQLiteStoreFixture {
  resetSQLiteStoreFixture()
      : SQLiteStoreFixture(true) {}
};

TEST_CASE_METHOD(resetSQLiteStoreFixture, "resetSQLiteStoreTests"){
    SECTION("setGet"){CHECK_MESSAGE_STORE_SET_GET}

    SECTION("setGetUint64"){CHECK_MESSAGE_STORE_SET_GET_UINT64}

    SECTION("setGetWithQuote"){CHECK_MESSAGE_STORE_SET_GET_WITH_QUOTE}

    SECTION("other"){CHECK_MESSAGE_STORE_OTHER}

    SECTION("otherUint64"){CHECK_MESSAGE_STORE_OTHER_UINT64}

    SET_SEQUENCE_NUMBERS}

TEST_CASE_METHOD(noResetSQLiteStoreFixture, "noResetSQLiteStoreTests") {
  SECTION("reload"){CHECK_MESSAGE_STORE_RELOAD}

  SECTION("refresh") {
    CHECK_MESSAGE_STORE_RELOAD
  }
}

#endif // HAVE_SQLITE3
