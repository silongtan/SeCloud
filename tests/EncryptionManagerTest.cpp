#include <gtest/gtest.h>

#include "../src/EncryptionManager.h"

void print_vector(const std::array<uint8_t, BLOCK_SIZE>& vec) {
    for (auto& v : vec) {
        std::cout << std::hex << (int)v << " ";
    }
    std::cout << std::endl;
}

// Demonstrate some basic assertions.
TEST(EncryptionManager, EncryptDecrypt) {
    std::array<uint8_t, KEY_SIZE> key = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                         0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
                                         0x0d, 0x0e, 0x0f, 0x10};
    std::array<uint8_t, USER_IV_SIZE> iv = {0x01, 0x02, 0x03, 0x04,
                                            0x05, 0x06, 0x07, 0x08};
    EncryptionManager emgr(key, iv);
    std::array<uint8_t, BLOCK_SIZE> block{};
    for (int i = 0; i < 4096; i++) {
        block[i] = i % 16;
    }
    auto encrypted_block = emgr.encrypt_block(block, 0);
    print_vector(encrypted_block);
    auto decrypted_block = emgr.decrypt_block(encrypted_block, 0);
    print_vector(decrypted_block);
    ASSERT_EQ(block, decrypted_block);

    encrypted_block = emgr.encrypt_block(block, 1);
    print_vector(encrypted_block);
    decrypted_block = emgr.decrypt_block(encrypted_block, 1);
    print_vector(decrypted_block);
    ASSERT_EQ(block, decrypted_block);
}