#include <vector>

#include "engine.h"

class PollEngine : public Engine {
 public:
  explicit PollEngine(int port);
  ~PollEngine() override;

  void run() override;

 private:
  void acceptNewConnections();
  void manageConnections();

 private:
  std::vector<Client> m_Clients;
};
