#include "PasswordManager.h"

#include <openssl/evp.h>

#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <fstream>
#include <iostream>
#include <utility>

#include "consts.h"
#include "openssl/rand.h"
#include "openssl/sha.h"

using namespace std;

std::string PasswordManager::get_password() { return password; }

bool PasswordManager::check_password() {
    std::cout << "Enter password for SeCloud: ";
    std::cin >> password;

    // read salt and hash from file
    std::ifstream file(password_path, std::ios::binary);
    if (!file.is_open()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to open password file" << std::endl;
        return false;
    }
    std::vector<uint8_t> salt(SALT_SIZE);
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    file.read(reinterpret_cast<char *>(salt.data()), SALT_SIZE);
    file.read(reinterpret_cast<char *>(hash.data()), SHA256_DIGEST_LENGTH);
    file.close();

    return hash == hash_password(password, salt);
}

void PasswordManager::set_password() {
    std::cout << "Setting up SeCloud. Enter new password: ";
    std::cin >> password;
    store_password(password);
}

PasswordManager::PasswordManager(std::string password_path)
    : password_path(std::move(password_path)) {}

std::vector<uint8_t> PasswordManager::generate_salt() {
    std::vector<unsigned char> salt(SALT_SIZE);
    if (RAND_bytes(salt.data(), static_cast<int>(SALT_SIZE)) != 1) {
        throw std::runtime_error("Error generating random bytes for salt.");
    }
    return salt;
}

std::vector<uint8_t> PasswordManager::hash_password(
    const string &plain_password, const vector<uint8_t> &salt) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, salt.data(), salt.size());
    SHA256_Update(&sha256, plain_password.c_str(), plain_password.length());
    std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
    SHA256_Final(hash.data(), &sha256);
    return hash;
}

void PasswordManager::store_password(const string &plain_password) {
    std::vector<uint8_t> salt = generate_salt();
    std::vector<uint8_t> hash = hash_password(plain_password, salt);
    std::ofstream file(PASSWORD_FILE, std::ios::trunc);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char *>(salt.data()),
                   (long)salt.size());
        file.write(reinterpret_cast<const char *>(hash.data()),
                   (long)hash.size());
        file.close();
    } else {
        BOOST_LOG_TRIVIAL(error) << "Failed to open password file" << std::endl;
        throw std::runtime_error("Failed to open password file");
    }
}
