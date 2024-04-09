#ifndef UTILS_H
#define UTILS_H

#include "BackupServer.grpc.pb.h"
#include "EncryptionManager.h"
#include "consts.h"

namespace utils {
bool consistency_check(int img_fd, EncryptionManager &emgr,
                       const std::unique_ptr<Backup::Stub> &client_stub);

bool rebuild_remote(int img_fd, EncryptionManager &emgr,
                    const std::unique_ptr<Backup::Stub> &client_stub);

bool recover_local(int img_fd, EncryptionManager &emgr,
                   const std::unique_ptr<Backup::Stub> &client_stub);
}  // namespace utils

#endif