#include <iostream>

#include <unistd.h>

#include "base/command_line.h"
#include "base/files/scoped_file.h"
#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/time/time.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
// #include "etc/examples/mojo/mojom/bar.mojom.h"
#include "etc/examples/mojo/mojom/ipc_worker.mojom.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/invitation.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/platform_handle.h"

void OnConnectionError() {
  std::cout << "OnConnectionError fired" << std::endl;
}

void OnEventRecevied(mojo::Remote<etc::mojom::IPCWorker>* ipc_worker,
                     etc::mojom::EventPtr pevent) {
  if (!pevent || !pevent->client) {
    return;
  }

  int sd = 0;
  MojoPlatformHandle platform_handle;
  platform_handle.struct_size = sizeof(platform_handle);
  MojoResult result = MojoUnwrapPlatformHandle(
      pevent->client->sd.release().value(), nullptr, &platform_handle);
  if (result == MOJO_RESULT_OK) {
    sd = platform_handle.value;
  } else {
    std::cout << " Unwrap failed" << std::endl;
    return;
  }

  pevent->client->sd =
      mojo::WrapPlatformHandle(mojo::PlatformHandle(base::ScopedFD(sd)));

  if (pevent->client->state == etc::mojom::ClientState::kRead) {
    char buf[256];
    int r = read(sd, buf, sizeof(buf));
    if (r < 0) {
      std::cout << "some read error!\n";
      return;
    }

    if (r > 0) {
      pevent->buffer.assign(buf, buf + r);
      (*ipc_worker)->PostWrite(std::move(pevent));
    } else if (r == 0) {
      pevent->buffer.assign("");
      (*ipc_worker)->PostWrite(std::move(pevent));
    }
  } else if (pevent->client->state == etc::mojom::ClientState::kWrite) {
    write(sd, pevent->buffer.c_str(), pevent->buffer.size());
    (*ipc_worker)->PostRead(std::move(pevent));
  } else {
    std::cout << "unknown client state";
  }
}

void CheckWork(mojo::Remote<etc::mojom::IPCWorker>* ipc_worker,
               scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  (*ipc_worker)->GetEvent(base::BindOnce(&OnEventRecevied, ipc_worker));

  task_runner->PostDelayedTask(
      FROM_HERE, base::BindOnce(&CheckWork, ipc_worker, task_runner),
      base::Milliseconds(30));
}

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  mojo::core::Init();

  base::Thread ipc_thread("ipc thread");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessagePumpType::IO, 0));

  // As long as this object is alive, all Mojo API surface relevant to IPC
  // connections is usable, and message pipes which span a process boundary will
  // continue to function.
  mojo::core::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  mojo::IncomingInvitation invitation = mojo::IncomingInvitation::Accept(
      mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(
          *command_line));
  mojo::ScopedMessagePipeHandle pipe =
      invitation.ExtractMessagePipe("Worker Pipe");

  //std::unique_ptr<base::MessageLoop> message_loop(new base::MessageLoop());
  base::SingleThreadTaskExecutor executor(base::MessagePumpType::IO);
  base::RunLoop run_loop;

  mojo::Remote<etc::mojom::IPCWorker> ipc_worker =
      mojo::Remote<etc::mojom::IPCWorker>(
          mojo::PendingRemote<etc::mojom::IPCWorker>(std::move(pipe),
                                                     /*version=*/0));

  ipc_worker.set_disconnect_handler(base::BindOnce(&OnConnectionError));

  executor.task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&CheckWork, &ipc_worker, executor.task_runner()));

  run_loop.Run();

  return 0;
}
