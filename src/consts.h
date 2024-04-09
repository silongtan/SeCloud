#ifndef CONST_H
#define CONST_H

constexpr size_t SPSC_SIZE = 1024;

constexpr uint64_t BLOCK_SIZE = 4096;

constexpr uint64_t N_BLOCKS = 1024;

constexpr uint64_t DEV_SIZE = BLOCK_SIZE * N_BLOCKS;

constexpr char IMG_FILE[] = "img";

constexpr char ENCRYPTED_IMG[] = "encrypted_backup_img";

constexpr char BLOCK_DEV[] = "/dev/nbd0";

constexpr char BACKUP_SERVER_ADDR[] = "localhost:8080";

constexpr size_t USER_IV_SIZE = 8;

#endif