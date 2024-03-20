#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <thread>

#include "BUSE/buse.h"
#include "consts.h"
#include "Daemon.h"
#include "EncryptionManager.h"

#define BOOST_LOG_DYN_LINK 1

int main(int, char**) {
    boost::log::add_console_log(std::cout,
                                boost::log::keywords::format = ">> %Message%");
    OperationQueue spsc;  // queue for communicating between daemon and buse
    EncryptionManager emgr("StoreDuke566!");
    std::atomic<bool> stop_flag(false);

    BOOST_LOG_TRIVIAL(info) << "SeCloud starts!" << std::endl;

    std::thread daemon(
        [&]() { Daemon::start(spsc, emgr, stop_flag); });  // start the deamon

    if (buse_main(nullptr, nullptr, nullptr) != EXIT_SUCCESS) {
        BOOST_LOG_TRIVIAL(fatal) << "Buse returns error" << std::endl;
    }

    stop_flag.store(true);
    daemon.join();
}
