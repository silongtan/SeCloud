#include "EncryptionManager.h"

#include <boost/log/trivial.hpp>

EncryptionManager::EncryptionManager(std::string key) {
    BOOST_LOG_TRIVIAL(info)
        << "Encyprtion manager created with key: " << key << std::endl;
}