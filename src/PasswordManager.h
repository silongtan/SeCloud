#include <string>
#include <vector>

#include "consts.h"
#include "openssl/base.h"

// Password manager that stores the plaintext of user and password
class PasswordManager final {
   private:
    std::string password;
    std::string password_path;
    static std::vector<uint8_t> generate_salt();
    static std::vector<uint8_t> hash_password(const std::string &plain_password,
                                              const std::vector<uint8_t> &salt);
    static void store_password(const std::string &plain_password);

   public:
    explicit PasswordManager(std::string password_path);
    std::string get_password();
    void set_password();
    bool check_password();
};