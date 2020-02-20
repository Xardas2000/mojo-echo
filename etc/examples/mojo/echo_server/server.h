#include <memory>

#include "engine.h"
#include "options.h"

class Server {
 public:
  Server(const Options& opt);
  ~Server();
  void run();

 private:
  std::unique_ptr<Engine> m_Engine;
};
