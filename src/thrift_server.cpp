#include "SlidingHyperService.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <syslog.h>

#include "config.h"
#include "set_manager.h"
#include "thrift_server.h"

using namespace apache::thrift::concurrency;
using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class SlidingHyperServiceHandler : virtual public SlidingHyperServiceIf {
 private:
  hlld_setmgr *mgr;
 public:
  SlidingHyperServiceHandler(hlld_setmgr *mgr) {
      assert(mgr != NULL);
      this->mgr = mgr;
  }

  void ping(std::string& _return) {
      _return = "PONG";
  }

  void add_many(const int32_t timestamp, const std::string& key, const std::vector<std::string> & values) {
      setmgr_client_checkpoint(mgr);

      char **char_v = (char**)malloc(sizeof(char*)*values.size());
      for(size_t i=0; i<values.size(); i++) {
          char_v[i] = (char*)&values[i][0];
      }

      int res = setmgr_set_keys(mgr, (char*)&key[0], char_v, values.size(), (time_t)timestamp);
      // set does not exist
      if (res == -1 ) {
          setmgr_create_set(mgr, (char*)&key[0], NULL);
          res = setmgr_set_keys(mgr, (char*)&key[0], char_v, values.size(), (time_t)timestamp);
      }
      else if (res < -1) {
          syslog(LOG_ERR, "Failure to add to key %s with value %s res: %d", (char*)&key[0], res);
      }
      free(char_v);
  }

  int32_t card(const int32_t timestamp, const int32_t window, const std::vector<std::string> & keys, const std::vector<std::string> & values) {
      //add(timestamp, 
      //return get_union(timestamp, window, keys);
  }

  void flush() {
    // Your implementation goes here
    printf("flush\n");
  }

  void add(const int32_t timestamp, const std::string& key, const std::string& value) {
      char *values[] = {(char*)&value[0]};
      int res = setmgr_set_keys(mgr, (char*)&key[0], values, 1, (time_t)timestamp);
      // set does not exist
      if (res == -1 ) {
          setmgr_create_set(mgr, (char*)&key[0], NULL);
          res = setmgr_set_keys(mgr, (char*)&key[0], values, 1, (time_t)timestamp);
      }
      else if (res < -1) {
          syslog(LOG_ERR, "Failure to add to key %s with value %s res: %d", (char*)&key[0], res);
      }

  }

  int32_t get(const int32_t timestamp, const int16_t window, const std::string& key) {
      uint64_t estimate = 0;
      int res = setmgr_set_size(mgr, (char*)&key[0], &estimate, window);
      if (res == -1) {
          res = setmgr_create_set(mgr, (char*)&key[0], NULL);
          return 0;
      } else if (res < -1) {
          syslog(LOG_ERR, "Failed to get set cardinality %s res %d", (char*)&key[0], res);
      }
      return estimate;
  }

  int32_t get_union(const int32_t timestamp, const int16_t window, const std::vector<std::string> & keys) {
      char **set_names = (char**)malloc(keys.size()*sizeof(char**));
      for(size_t i=0; i<keys.size(); i++) {
          set_names[i] = (char*)&keys[i];
      }
      uint64_t estimate;
      setmgr_set_union_size(mgr, keys.size(), set_names, &estimate, window);
      free(set_names);

      return (int32_t)estimate;

  }

  int32_t get_with_element(const int32_t timestamp, const int16_t window, const std::string& key, const std::string& value) {
    // Your implementation goes here
    printf("get_withelement\n");
  }

  int32_t get_union_with_element(const int32_t timestamp, const int16_t window, const std::vector<std::string> & keys, const std::string& value) {
    // Your implementation goes here
    printf("get_union_with_element\n");
  }

};

TThreadPoolServer *thrift_server;


void start_thrift_server(hlld_setmgr *mgr) {
  int port = 9090;
  shared_ptr<SlidingHyperServiceHandler> handler(new SlidingHyperServiceHandler(mgr));
  shared_ptr<TProcessor> processor(new SlidingHyperServiceProcessor(handler));
  shared_ptr<TServerSocket> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

 shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(4);

 shared_ptr<PosixThreadFactory> threadFactory
     = shared_ptr<PosixThreadFactory>(new PosixThreadFactory());

 threadManager->threadFactory(threadFactory);

 threadManager->start();

  thrift_server = new TThreadPoolServer(processor, serverTransport, transportFactory, protocolFactory, threadManager);
  syslog(LOG_INFO, "Starting thrift server");
  thrift_server->serve();
  syslog(LOG_INFO, "Stopping thrift server");
}

void stop_thrift_server() {
    thrift_server->stop();
}
