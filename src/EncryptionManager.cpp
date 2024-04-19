#include "EncryptionManager.h"

#include <openssl/evp.h>

#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <fstream>
#include <utility>

#include "consts.h"

EncryptionManager::EncryptionManager(std::array<uint8_t, KEY_SIZE> key,
                                     std::array<uint8_t, USER_IV_SIZE> iv)
    : key(key), user_iv(iv) {}

std::array<uint8_t, BLOCK_SIZE> EncryptionManager::encrypt_block(
    const std::array<uint8_t, BLOCK_SIZE>& block, uint64_t block_no) {
    BOOST_ASSERT_MSG(block.size() == BLOCK_SIZE,
                     "Block size must be equal to BLOCK_SIZE");
    BOOST_LOG_TRIVIAL(debug) << "Encrypting block " << block_no << std::endl;

    auto ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        BOOST_LOG_TRIVIAL(error)
            << "Failed to create cipher context" << std::endl;
        throw std::runtime_error("Failed to create cipher context");
    }

    auto block_iv = gen_iv_from_block_no(block_no);

    if (1 !=
        EVP_EncryptInit(ctx, EVP_aes_256_ctr(), key.data(), block_iv.data())) {
        BOOST_LOG_TRIVIAL(error)
            << "Failed to initialize encryption" << std::endl;
        throw std::runtime_error("Failed to initialize encryption");
    }

    std::array<uint8_t, BLOCK_SIZE> encrypted_block{};
    int out_len1;
    if (1 != EVP_EncryptUpdate(ctx, encrypted_block.data(), &out_len1,
                               block.data(), (int)block.size())) {
        BOOST_LOG_TRIVIAL(error) << "Failed to encrypt update" << std::endl;
    }
    BOOST_ASSERT_MSG(out_len1 == BLOCK_SIZE,
                     "Encrypted block size must be equal to BLOCK_SIZE");

    int out_len2 = int(encrypted_block.size()) - out_len1;
    if (1 !=
        EVP_EncryptFinal(ctx, encrypted_block.data() + out_len1, &out_len2)) {
        BOOST_LOG_TRIVIAL(error)
            << "Failed to finalize encryption" << std::endl;
    }
    BOOST_ASSERT_MSG(out_len2 == 0,
                     "Encrypted block finish size must be equal to 0");

    EVP_CIPHER_CTX_free(ctx);

    return encrypted_block;
}

std::array<uint8_t, BLOCK_SIZE> EncryptionManager::decrypt_block(
    const std::array<uint8_t, BLOCK_SIZE>& block, uint64_t block_no) {
    BOOST_ASSERT_MSG(block.size() == BLOCK_SIZE,
                     "Block size must be equal to BLOCK_SIZE");
    BOOST_LOG_TRIVIAL(debug) << "Decrypting block " << block_no << std::endl;

    auto ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        BOOST_LOG_TRIVIAL(error)
            << "Failed to create cipher context" << std::endl;
        throw std::runtime_error("Failed to create cipher context");
    }

    auto block_iv = gen_iv_from_block_no(block_no);

    if (1 !=
        EVP_DecryptInit(ctx, EVP_aes_256_ctr(), key.data(), block_iv.data())) {
        BOOST_LOG_TRIVIAL(error)
            << "Failed to initialize decryption" << std::endl;
        throw std::runtime_error("Failed to initialize decryption");
    }

    std::array<uint8_t, BLOCK_SIZE> decrypted_block{};
    int out_len1 = (int)decrypted_block.size();

    if (1 != EVP_DecryptUpdate(ctx, decrypted_block.data(), &out_len1,
                               block.data(), (int)block.size())) {
        BOOST_LOG_TRIVIAL(error) << "Failed to decrypt update" << std::endl;
    }
    BOOST_ASSERT_MSG(out_len1 == BLOCK_SIZE,
                     "Decrypted block size must be equal to BLOCK_SIZE");

    int out_len2 = int(decrypted_block.size()) - out_len1;
    if (1 !=
        EVP_DecryptFinal(ctx, decrypted_block.data() + out_len1, &out_len2)) {
        BOOST_LOG_TRIVIAL(error)
            << "Failed to finalize decryption" << std::endl;
    }
    BOOST_ASSERT_MSG(out_len2 == 0,
                     "Decrypted block finish size must be equal to 0");

    EVP_CIPHER_CTX_free(ctx);

    return decrypted_block;
}

std::vector<uint8_t> EncryptionManager::gen_iv_from_block_no(
    uint64_t block_no) {
    auto augmented_block_no = block_no * BLOCK_SIZE / 16;

    std::vector<uint8_t> new_iv(user_iv.begin(), user_iv.end());
    for (int i = 0; i < sizeof(augmented_block_no); i++) {
        new_iv.push_back(augmented_block_no >> (i * 8) & 0xFF);
    }

    std::reverse(new_iv.end() - sizeof(augmented_block_no), new_iv.end());
    return new_iv;
}

EncryptionManager::EncryptionManager(std::string password)
    : key{}, user_iv({0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}) {
    for (int i = 0; i < password.size() && i < key.size(); i++) {
        key[i] = password[i];
    }
}
