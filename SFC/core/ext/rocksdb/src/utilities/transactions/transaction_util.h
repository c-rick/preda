// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#ifndef ROCKSDB_LITE

#include <string>
#include <unordered_map>

#include "../../db/dbformat.h"
#include "../../db/read_callback.h"
#include "../../include/db.h"
#include "../../include/slice.h"
#include "../../include/status.h"
#include "../../include/types.h"
#include "../../utilities/transactions/lock/lock_tracker.h"

namespace ROCKSDB_NAMESPACE {

class DBImpl;
struct SuperVersion;
class WriteBatchWithIndex;

class TransactionUtil {
 public:
  // Verifies there have been no commits to this key in the db since this
  // sequence number.
  //
  // If cache_only is true, then this function will not attempt to read any
  // SST files.  This will make it more likely this function will
  // return an error if it is unable to determine if there are any conflicts.
  //
  // See comment of CheckKey() for explanation of `snap_seq`, `snap_checker`
  // and `min_uncommitted`.
  //
  // Returns OK on success, BUSY if there is a conflicting write, or other error
  // status for any unexpected errors.
  static Status CheckKeyForConflicts(
      DBImpl* db_impl, ColumnFamilyHandle* column_family,
      const std::string& key, SequenceNumber snap_seq, bool cache_only,
      ReadCallback* snap_checker = nullptr,
      SequenceNumber min_uncommitted = kMaxSequenceNumber);

  // For each key,SequenceNumber pair tracked by the LockTracker, this function
  // will verify there have been no writes to the key in the db since that
  // sequence number.
  //
  // Returns OK on success, BUSY if there is a conflicting write, or other error
  // status for any unexpected errors.
  //
  // REQUIRED:
  // This function should only be called on the write thread or if the
  // mutex is held.
  // tracker must support point lock.
  static Status CheckKeysForConflicts(DBImpl* db_impl,
                                      const LockTracker& tracker,
                                      bool cache_only);

 private:
  // If `snap_checker` == nullptr, writes are always commited in sequence number
  // order. All sequence number <= `snap_seq` will not conflict with any
  // write, and all keys > `snap_seq` of `key` will trigger conflict.
  // If `snap_checker` != nullptr, writes may not commit in sequence number
  // order. In this case `min_uncommitted` is a lower bound.
  //  seq < `min_uncommitted`: no conflict
  //  seq > `snap_seq`: applicable to conflict
  //  `min_uncommitted` <= seq <= `snap_seq`: call `snap_checker` to determine.
  static Status CheckKey(DBImpl* db_impl, SuperVersion* sv,
                         SequenceNumber earliest_seq, SequenceNumber snap_seq,
                         const std::string& key, bool cache_only,
                         ReadCallback* snap_checker = nullptr,
                         SequenceNumber min_uncommitted = kMaxSequenceNumber);
};

}  // namespace ROCKSDB_NAMESPACE

#endif  // ROCKSDB_LITE
