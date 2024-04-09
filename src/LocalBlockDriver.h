#ifndef LOCAL_BLOCK_DRIVER_H
#define LOCAL_BLOCK_DRIVER_H

#include <cstdint>

#include "AsyncOperationQueue.h"
#include "BackupDaemon.h"

namespace LocalBlockDriver {

struct Context {
    std::shared_ptr<AsyncOperationQueue> queue;
    int fd{};
};

int read(void *buf, uint32_t len, uint64_t offset, void *userdata);

int write(const void *buf, uint32_t len, uint64_t offset, void *userdata);

int flush(void *userdata);

void disc(void *userdata);

int trim(uint64_t from, uint32_t len, void *userdata);

}  // namespace LocalBlockDriver

#endif