#include "server.h"

#include <iostream>

#include "poll.h"
#include "poll_async.h"
#include "poll_mojo.h"
#include "select.h"

namespace {
std::unique_ptr<Engine> get_engine(engine_t type, int port, bool async) {
  std::unique_ptr<Engine> ret;
  switch (type) {
    case engine_t::SELECT:
      ret.reset(new SelectEngine(port));
      break;
    case engine_t::POLL: {
      if (async)
        ret.reset(new AsyncPollEngine(port));
      else
        ret.reset(new PollEngine(port));
      break;
    }
    case engine_t::MOJO:
      ret.reset(new etc::MojoEngine(port));
      break;
    case engine_t::UNKNOWN:
      std::cerr << "unknown engine type";
  }

  return ret;
}
}  // namespace

Server::Server(const Options& opt) {
  m_Engine = get_engine(opt.engine(), opt.port(), opt.async());
}
Server::~Server() = default;

void Server::run() {
  m_Engine->run();
}
