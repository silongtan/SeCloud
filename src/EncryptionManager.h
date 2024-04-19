#ifndef ENCRYPTION_MANAGER_H
#define ENCRYPTION_MANAGER_H
#include <string>
#include <vector>

#include "consts.h"
#include "openssl/base.h"

class EncryptionManager final {
   private:
    std::array<uint8_t, KEY_SIZE> key;
    std::array<uint8_t, USER_IV_SIZE> user_iv{};

    std::vector<uint8_t> gen_iv_from_block_no(uint64_t block_no);

   public:
    explicit EncryptionManager(std::string password);
    explicit EncryptionManager(std::array<uint8_t, KEY_SIZE> key,
                               std::array<uint8_t, USER_IV_SIZE> iv);
    std::array<uint8_t, BLOCK_SIZE> encrypt_block(
        const std::array<uint8_t, BLOCK_SIZE>& block, uint64_t block_no);
    std::array<uint8_t, BLOCK_SIZE> decrypt_block(
        const std::array<uint8_t, BLOCK_SIZE>& block, uint64_t block_no);
};

#endif