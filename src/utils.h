#ifndef UTILS_H
#define UTILS_H

#include "BackupServer.grpc.pb.h"
#include "EncryptionManager.h"
#include "consts.h"
#include "types.h"

namespace utils {
bool consistency_check(int img_fd, EncryptionManager &emgr,
                       const std::unique_ptr<Backup::Stub> &client_stub,
                       const Config &config);

bool rebuild_remote(int img_fd, EncryptionManager &emgr,
                    const std::unique_ptr<Backup::Stub> &client_stub,
                    const Config &config);

bool recover_local(int img_fd, EncryptionManager &emgr,
                   const std::unique_ptr<Backup::Stub> &client_stub,
                   const Config &config);
Config parse_options(int argc, char *argv[]);

}  // namespace utils

#endif