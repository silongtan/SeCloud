#ifndef DAEMON_H
#define DAEMON_H

#include <boost/lockfree/spsc_queue.hpp>
#include <memory>

#include "AsyncOperationQueue.h"
#include "BackupServer.grpc.pb.h"
#include "EncryptionManager.h"

typedef std::atomic<bool> StopFlag;

class BackupDaemon {
   public:
    static void start(const std::shared_ptr<AsyncOperationQueue>& queue,
                      int img_fd, EncryptionManager& emgr,
                      const std::unique_ptr<Backup::Stub>& client_stub,
                      const StopFlag& stop);
};

#endif