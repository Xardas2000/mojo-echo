#ifndef ETC_EXAMPLES_MOJO_ECHO_SERVER_POLL_MOJO_H_
#define ETC_EXAMPLES_MOJO_ECHO_SERVER_POLL_MOJO_H_

#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "engine.h"

class EventLoop {
 public:
  ~EventLoop();

  static EventLoop& eventLoop() {
    static EventLoop ev;
    return ev;
  }
  void run();

  void asyncRead(int sd, std::string& str, std::function<void(int)> cb);
  void asyncWrite(int sd, const std::string& str, std::function<void(int)> cb);

 private:
  EventLoop();

  int manageConnections();  // push into queue READ or WRITE events
  void deleteClient(int sd);

  struct Event {
    Event(Client c, std::string& data);
    ~Event();
    Event(const Event& rhs);

    Event& operator=(const Event& h);

    Client _client;
    std::function<void(int)> _callback;
    std::reference_wrapper<std::string> _data;  // read or write buffer
  };

 private:
  std::mutex m_HaveWorkQueueMutex;
  std::mutex m_WantWorkQueueMutex;

  std::vector<Event> m_ClientsWantWork;  // clients who wants read or write
  std::queue<Event> m_ClientsHaveWork;  // clients who has data to read or write
};

class Connection : public std::enable_shared_from_this<Connection> {
 public:
  explicit Connection(int sd, EventLoop* loop);
  ~Connection();

  Connection(const Connection& rhs) = delete;
  Connection& operator=(const Connection& rhs) = delete;

  void read();
  void write(const std::string& s);

 private:
  void readHandler(int error);
  void writeHandler(int error);

  std::string m_WriteBuf;
  std::string m_ReadBuf;

  int m_Sd;
  EventLoop* m_EventLoop;
};

class AsyncPollEngine : public Engine {
 public:
  explicit AsyncPollEngine(int port) : Engine(port) {}
  void run() override;
};

#endif  // ETC_EXAMPLES_MOJO_ECHO_SERVER_POLL_ASYNC_H_