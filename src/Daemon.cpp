#include "Daemon.h"

#include <boost/log/trivial.hpp>

void Daemon::start(OperationQueue &spsc, EncryptionManager &emgr,
                   StopFlag &stop) {
    BOOST_LOG_TRIVIAL(info) << "Daemon starts!" << std::endl;
}
