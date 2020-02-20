#include <vector>

#include "engine.h"

class SelectEngine : public Engine {
 public:
  explicit SelectEngine(int port);
  ~SelectEngine() override;

  void run() override;

 private:
  void manageConnections();

  std::vector<Client> m_Clients;
};
