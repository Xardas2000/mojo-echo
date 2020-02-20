// Copyright (c) 2020 Mail.Ru Groups Authors. All rights reserved.
// Owners: i.saneev@corp.mail.ru

#ifndef ETC_EXAMPLES_MOJO_ECHO_SERVER_POLL_MOJO_H_
#define ETC_EXAMPLES_MOJO_ECHO_SERVER_POLL_MOJO_H_

#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "etc/examples/mojo/echo_server/engine.h"
#include "etc/examples/mojo/mojom/ipc_worker.h"

namespace etc {

class EventLoop {
 public:
  ~EventLoop();

  static EventLoop& eventLoop() {
    static EventLoop ev;
    return ev;
  }

  void Run();

  void AsyncRead(mojom::EventPtr event);
  void AsyncWrite(mojom::EventPtr event);

  mojom::EventPtr GetEvent();

 private:
  EventLoop();

  int ManageConnections();  // push into queue READ or WRITE events
  void DeleteClient(int sd);

 private:
  std::mutex have_work_queue_mutex_;
  std::mutex want_work_queue_mutex_;

  std::vector<mojom::EventPtr>
      clients_want_work_;  // clients who wants read or write
  std::queue<mojom::EventPtr>
      clients_have_work_;  // clients who has data to read or write

  bool launched_ = false;
};

class MojoEngine : public Engine {
 public:
  explicit MojoEngine(int port) : Engine(port) {}
  void run() override;
};

}  // namespace etc

#endif  // ETC_EXAMPLES_MOJO_ECHO_SERVER_POLL_MOJO_H_
