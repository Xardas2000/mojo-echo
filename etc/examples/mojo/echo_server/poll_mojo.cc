#include "etc/examples/mojo/echo_server/poll_mojo.h"

#include <assert.h>
#include <errno.h>
#include <netinet/in.h>  // htons(), INADDR_ANY
#include <sys/poll.h>
#include <sys/socket.h>  // socket()

#include <stdint.h>  // uint32_t
#include <string.h>
#include <unistd.h>  // close()
#include <algorithm>
#include <iostream>
#include <map>
#include <utility>
#include <vector>
#include <thread>

// browser specific
#include "base/command_line.h"
#include "base/files/scoped_file.h"
#include "base/process/launch.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/invitation.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace etc {

EventLoop::EventLoop() = default;
EventLoop::~EventLoop() = default;

// EVENT LOOP

void EventLoop::DeleteClient(int sd) {
  std::unique_lock<std::mutex> lock(
      want_work_queue_mutex_);  // without this lock it fault very easy

  auto f = [&sd](const mojom::EventPtr& event) {
    return event && event->raw_handle == sd;
  };

  auto iterator =
      std::find_if(clients_want_work_.begin(), clients_want_work_.end(), f);

  std::cout << "delete: " << sd << "\n";
  assert(iterator != clients_want_work_.end());

  clients_want_work_.erase(iterator);
}

void EventLoop::AsyncRead(mojom::EventPtr event) {
  if (!event)
    return;
  std::unique_lock<std::mutex> lock(want_work_queue_mutex_);

  event->client->state = etc::mojom::ClientState::kRead;
  clients_want_work_.emplace_back(std::move(event));
}

void EventLoop::AsyncWrite(mojom::EventPtr event) {
  if (!event)
    return;
  std::unique_lock<std::mutex> lock(want_work_queue_mutex_);

  event->buffer = "server : " + event->buffer;
  event->client->state = etc::mojom::ClientState::kWrite;

  clients_want_work_.emplace_back(std::move(event));
}

mojom::EventPtr EventLoop::GetEvent() {
  std::unique_lock<std::mutex> lock(have_work_queue_mutex_);
  if (clients_have_work_.empty()) {
    return mojom::EventPtr();
  }

  mojom::EventPtr event = std::move(clients_have_work_.front());
  clients_have_work_.pop();
  return event;
}

/*
    push into _queue READ or WRITE events
*/
int EventLoop::ManageConnections() {
  std::cerr << "start connection manager thread: " << pthread_self()
            << std::endl;
  struct pollfd fds[32768];  // 2^15

  while (true) {
    std::vector<int> disconnected_clients;

    if (clients_want_work_.empty()) {
      // no clients
      usleep(1000);
      continue;
    }

    for (size_t i = 0; i < clients_want_work_.size(); ++i) {
      fds[i].fd = clients_want_work_[i]->raw_handle;

      if (clients_want_work_[i]->client->state ==
          etc::mojom::ClientState::kRead)
        fds[i].events = POLLIN;
      else
        fds[i].events = POLLOUT;
      fds[i].revents = 0;
    }

    int poll_ret =
        poll(fds, clients_want_work_.size(), /* timeout in msec */ 10);

    if (poll_ret == 0) {
      // nothing activity from any clients
      continue;
    } else if (poll_ret == -1) {
      std::cout << "poll error!\n";
      return -1;
    }

    for (size_t i = 0; i < clients_want_work_.size(); ++i) {
      if (fds[i].revents == 0)
        continue;

      if (fds[i].revents & POLLHUP) {
        // e.g. previous write() was in a already closed sd
        std::cout << "client hup\n";
        disconnected_clients.push_back(fds[i].fd);
      } else if (fds[i].revents & POLLIN) {
        if (clients_want_work_[i]->client->state !=
            etc::mojom::ClientState::kRead)
          continue;

        {
          std::unique_lock<std::mutex> lock(have_work_queue_mutex_);
          clients_have_work_.push(std::move(clients_want_work_[i]));
        }
        {
          std::unique_lock<std::mutex> lock(
              want_work_queue_mutex_);  // without this lock it fault very easy
          clients_want_work_.erase(clients_want_work_.begin() + i);
        }
        // DeleteClient(fds[i].fd);
      } else if (fds[i].revents & POLLOUT) {
        if (clients_want_work_[i]->client->state !=
            etc::mojom::ClientState::kWrite)
          continue;

        {
          std::unique_lock<std::mutex> lock(have_work_queue_mutex_);
          clients_have_work_.push(std::move(clients_want_work_[i]));
        }
        {
          std::unique_lock<std::mutex> lock(
              want_work_queue_mutex_);  // without this lock it fault very easy
          clients_want_work_.erase(clients_want_work_.begin() + i);
        }
        // DeleteClient(fds[i].fd);
      } else if (fds[i].revents & POLLNVAL) {
        // e.g. if set clos'ed descriptor in poll
        std::cerr << "POLLNVAL !!! need remove this descriptor: " << fds[i].fd
                  << std::endl;
        disconnected_clients.push_back(fds[i].fd);
      } else {
        if (fds[i].revents & POLLERR)
          std::cout << "WARNING> revents = POLLERR. [SD = " << fds[i].fd << "]"
                    << std::endl;
        else
          std::cout << "WARNIG> revent = UNKNOWN_EVENT: " << fds[i].revents
                    << " [SD = " << fds[i].fd << "]" << std::endl;
      }
    }

    // remove disconnected clients
    for (size_t i = 0; i < disconnected_clients.size(); ++i) {
      DeleteClient(disconnected_clients[i]);
    }
  }
  return 0;
}

void EventLoop::Run() {
  if (launched_)
    return;
  std::thread conn_manager_th(&EventLoop::ManageConnections, this);
  conn_manager_th.detach();
  launched_ = true;
}

mojom::EventPtr accept_work(int sd) {
  int timeout = 3000;  // msec
  struct pollfd fd;
  fd.fd = sd;
  fd.events = POLLIN;
  fd.revents = 0;

  while (1) {
    int poll_ret = poll(&fd, 1, /* timeout in msec */ timeout);

    if (poll_ret == 0) {
      continue;
    } else if (poll_ret == -1) {
      std::cout << "poll error" << std::endl;
    }

    struct sockaddr_in client;
    memset(&client, 0, sizeof(client));
    socklen_t cli_len = sizeof(client);
    int cli_sd = accept(sd, (struct sockaddr*)&client, &cli_len);
    base::ScopedFD sock(cli_sd);
    mojom::EventPtr e = mojom::Event::New();
    e->raw_handle = cli_sd;
    e->client = mojom::Client::New(
        mojo::WrapPlatformHandle(mojo::PlatformHandle(std::move(sock))),
        mojom::ClientState::kRead);
    std::cout << "+Connection: " << cli_sd << "\n";
    return e;
  }
}

void MojoEngine::run() {
  std::cout << "mojo poll server starts" << std::endl;

  EventLoop& ev = EventLoop::eventLoop();
  ev.Run();

  while (true) {
    mojom::EventPtr event = accept_work(listener());
    ev.AsyncRead(std::move(event));
  }
}
}  // namespace etc
