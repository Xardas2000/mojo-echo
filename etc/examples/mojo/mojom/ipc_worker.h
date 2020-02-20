#ifndef ETC_EXAMPLES_MOJO_MOJOM_IPC_WORKER_H_
#define ETC_EXAMPLES_MOJO_MOJOM_IPC_WORKER_H_

#include "etc/examples/mojo/mojom/ipc_worker.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace etc {

class EventLoop;

class IPCWorkerImpl : public etc::mojom::IPCWorker {
 public:
  explicit IPCWorkerImpl(mojo::PendingReceiver<etc::mojom::IPCWorker> receiver);

  ~IPCWorkerImpl() override;

  void GetEvent(GetEventCallback callback) override;

  void PostRead(mojom::EventPtr event) override;

  void PostWrite(mojom::EventPtr event) override;

 private:
  IPCWorkerImpl(const IPCWorkerImpl&) = delete;
  IPCWorkerImpl& operator=(const IPCWorkerImpl&) = delete;

  mojo::Receiver<etc::mojom::IPCWorker> receiver_;
  EventLoop* event_loop_;
};

}  // namespace etc

#endif  // ETC_EXAMPLES_MOJO_MOJOM_IPC_WORKER_H_
