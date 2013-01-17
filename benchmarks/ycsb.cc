#include <iostream>

#include "../macros.h"
#include "../varkey.h"
#include "../thread.h"
#include "../util.h"

#include "bdb_wrapper.h"
#include "ndb_wrapper.h"

using namespace std;
using namespace util;

//static const size_t nkeys = 1000;
static const size_t nkeys = 140000000;
static size_t nthreads = 1;
static volatile bool running = true;

class worker : public ndb_thread {
public:
  worker(unsigned long seed, abstract_db *db) : seed(seed), db(db), ntxns(0) {}

  virtual void run()
  {
    fast_random r(seed);
    while (running) {
      void *txn = db->new_txn();
      string k = u64_varkey(r.next()).str();
      char *v = 0;
      size_t vlen = 0;
      bool found = db->get(txn, k.data(), k.size(), v, vlen);
      if (!db->commit_txn(txn))
        continue;
      ntxns++;
      if (found)
        free(v);
    }
  }

  unsigned long seed;
  abstract_db *db;
  size_t ntxns;
};

static void
do_test(abstract_db *db)
{
  // load
  for (size_t i = 0; i < nkeys; i++) {
    void *txn = db->new_txn();
    string k = u64_varkey(i).str();
    string v(128, 'a');
    db->put(txn, k.data(), k.size(), v.data(), v.size());
    ALWAYS_ASSERT(db->commit_txn(txn));
  }

  fast_random r(8544290);
  vector<worker *> workers;
  for (size_t i = 0; i < nthreads; i++)
    workers.push_back(new worker(r.next(), db));
  timer t;
  for (size_t i = 0; i < nthreads; i++)
    workers[i]->start();
  sleep(10);
  running = false;
  size_t n = 0;
  for (size_t i = 0; i < nthreads; i++) {
    workers[i]->join();
    n += workers[i]->ntxns;
  }

  double agg_throughput = double(n) / (double(t.lap()) / 1000000.0);
  double avg_per_core_throughput = agg_throughput / double(nthreads);

  cerr << "agg_read_throughput: " << agg_throughput << " gets/sec" << endl;
  cerr << "avg_per_core_read_throughput: " << avg_per_core_throughput << " gets/sec/core" << endl;
}

int main(void)
{
  //int ret UNUSED = system("rm -rf db/*");
  //bdb_wrapper w("db", "ycsb.db");

  ndb_wrapper w;

  do_test(&w);
  return 0;
}
