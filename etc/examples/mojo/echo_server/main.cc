#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "base/command_line.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/threading/thread.h"
#include "etc/examples/mojo/mojom/ipc_worker.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/invitation.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/system/platform_handle.h"

#include "etc/examples/mojo/echo_server/server.h"

constexpr int kWorkersAmount = 4;

mojo::ScopedMessagePipeHandle LaunchProcessAndGetPipe() {
  mojo::PlatformChannel channel;

  mojo::OutgoingInvitation invitation;

  mojo::ScopedMessagePipeHandle pipe =
      invitation.AttachMessagePipe("Worker Pipe");

  base::LaunchOptions options;
  base::CommandLine command_line(
      base::FilePath(FILE_PATH_LITERAL("./echo_worker")));
  channel.PrepareToPassRemoteEndpoint(&options, &command_line);
  base::Process child_process = base::LaunchProcess(command_line, options);
  channel.RemoteProcessLaunchAttempted();

  mojo::OutgoingInvitation::Send(std::move(invitation), child_process.Handle(),
                                 channel.TakeLocalEndpoint());

  return pipe;
}

int main(int argc, char* argv[]) {
  std::cout << "SERVER my current pid is " << getpid() << std::endl;
  mojo::core::Init();

  base::Thread ipc_thread("ipc!");
  ipc_thread.StartWithOptions(
      base::Thread::Options(base::MessagePumpType::IO, /*size = */ 0));

  mojo::core::ScopedIPCSupport ipc_support(
      ipc_thread.task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  // bind our mojom interface
  base::SingleThreadTaskExecutor executor(base::MessagePumpType::IO);
  //std::unique_ptr<base::MessageLoop> message_loop(new base::MessageLoop());
  base::RunLoop run_loop;

  std::vector<std::unique_ptr<etc::IPCWorkerImpl>> ipc_workers;
  for (int i = 0; i < kWorkersAmount; ++i) {
    auto pipe = LaunchProcessAndGetPipe();
    auto receiver =
        mojo::PendingReceiver<etc::mojom::IPCWorker>(std::move(pipe));
    auto ipc_worker = std::make_unique<etc::IPCWorkerImpl>(std::move(receiver));
    ipc_workers.push_back(std::move(ipc_worker));
  }

  auto initf = [&]() {
    std::cout << "Child thread is " << base::PlatformThread::CurrentId()
              << std::endl;
    Options opt(argc, argv);
    Server serv(opt);
    serv.run();
  };

  std::thread t(initf);
  t.detach();

  run_loop.Run();

  return 0;
}
