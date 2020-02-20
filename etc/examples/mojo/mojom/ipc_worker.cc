// Copyright (c) 2020 Mail.Ru Groups Authors. All rights reserved.
// Owners: i.saneev@corp.mail.ru
//
#include "etc/examples/mojo/mojom/ipc_worker.h"

#include <unistd.h>
#include <iostream>

#include "base/files/scoped_file.h"
#include "etc/examples/mojo/echo_server/poll_mojo.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace etc {

IPCWorkerImpl::IPCWorkerImpl(
    mojo::PendingReceiver<etc::mojom::IPCWorker> receiver)
    : receiver_(this, std::move(receiver)),
      event_loop_(&EventLoop::eventLoop()) {
  DCHECK(event_loop_);
}

IPCWorkerImpl::~IPCWorkerImpl() {}

void IPCWorkerImpl::GetEvent(GetEventCallback callback) {
  std::move(callback).Run(event_loop_->GetEvent());
}

void IPCWorkerImpl::PostRead(mojom::EventPtr event) {
  event_loop_->AsyncRead(std::move(event));
}

void IPCWorkerImpl::PostWrite(mojom::EventPtr event) {
  event_loop_->AsyncWrite(std::move(event));
}

}  // namespace etc
