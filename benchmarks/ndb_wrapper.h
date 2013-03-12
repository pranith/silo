#ifndef _NDB_WRAPPER_H_
#define _NDB_WRAPPER_H_

#include "abstract_db.h"
#include "../txn_btree.h"

class ndb_wrapper : public abstract_db {
public:

  enum Proto {
    PROTO_1,
    PROTO_2,
  };

  ndb_wrapper(Proto proto) : proto(proto) {}

  virtual void
  do_txn_epoch_sync() const
  {
    switch (proto) {
    case PROTO_1:
      txn_epoch_sync<transaction_proto1>::sync();
      break;
    case PROTO_2:
      txn_epoch_sync<transaction_proto2>::sync();
      break;
    default:
      ALWAYS_ASSERT(false);
      break;
    }
  }

  virtual void
  do_txn_finish() const
  {
    switch (proto) {
    case PROTO_1:
      txn_epoch_sync<transaction_proto1>::finish();
      break;
    case PROTO_2:
      txn_epoch_sync<transaction_proto2>::finish();
      break;
    default:
      ALWAYS_ASSERT(false);
      break;
    }
  }

  virtual void *new_txn(uint64_t txn_flags);
  virtual bool commit_txn(void *txn);
  virtual void abort_txn(void *txn);
  virtual void print_txn_debug(void *txn) const;

  virtual abstract_ordered_index *
  open_index(const std::string &name, size_t value_size_hint);

  virtual void
  close_index(abstract_ordered_index *idx);

private:
  Proto proto;
};

class ndb_ordered_index : public abstract_ordered_index {
public:
  ndb_ordered_index(const std::string &name, size_t value_size_hint);
  virtual bool get(
      void *txn,
      const std::string &key,
      std::string &value, size_t max_bytes_read);
  virtual const char * put(
      void *txn,
      const std::string &key,
      const std::string &value);
  virtual const char * put(
      void *txn,
      std::string &&key,
      std::string &&value);
  virtual void scan(
      void *txn,
      const std::string &start_key,
      const std::string *end_key,
      scan_callback &callback);
  virtual void remove(
      void *txn,
      const std::string &key);
  virtual void remove(
      void *txn,
      std::string &&key);
  virtual size_t size() const;
  virtual void clear();
private:
  std::string name;
  txn_btree btr;
};

#endif /* _NDB_WRAPPER_H_ */
