#ifndef ENCRYPTION_MANAGER_H
#define ENCRYPTION_MANAGER_H
#include <string>
#include <vector>

class EncryptionManager {
   public:
    explicit EncryptionManager(std::string key);
    std::vector<uint8_t> encrypt_block(std::vector<uint8_t> block,
                                       uint64_t block_no);
    std::vector<uint8_t> decrypt_block(std::vector<uint8_t> block,
                                       uint64_t block_no);
};

#endif