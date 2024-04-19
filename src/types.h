#ifndef SECLOUD_TYPES_H
#define SECLOUD_TYPES_H
#include <string>

#include "consts.h"

enum Mode { SETUP, RECOVER_LOCAL, REBUILD_BACKUP, NORMAL };

struct Config {
    Mode mode = Mode::NORMAL;
    bool check = false;
    uint64_t size = DEV_SIZE;  // 4MB
    std::string file = IMG_FILE;
    uint64_t n_blocks = N_BLOCKS;
    size_t queue_size = SPSC_SIZE;
    bool verbose = false;
    std::string backup_server = BACKUP_SERVER_ADDR;
};

#endif  // SECLOUD_TYPES_H
