#include <fcntl.h>
#include <grpcpp/create_channel.h>

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <thread>

#include "BUSE/buse.h"
#include "BackupDaemon.h"
#include "EncryptionManager.h"
#include "LocalBlockDriver.h"
#include "PasswordManager.h"
#include "grpcpp/security/credentials.h"
#include "utils.h"

using grpc::Channel;

int main(int argc, char* argv[]) {
    boost::log::add_console_log(std::cout,
                                boost::log::keywords::format = ">> %Message%");

    auto config = utils::parse_options(argc, argv);

    if (!config.verbose) {
        boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                            boost::log::trivial::info);
    }

    // check password
    PasswordManager pm(PASSWORD_FILE);
    if (!std::filesystem::exists(PASSWORD_FILE) || config.mode == Mode::SETUP) {
        config.mode = Mode::SETUP;
        pm.set_password();
    } else if (!pm.check_password()) {
        std::cout << "Incorrect password" << std::endl;
        return EXIT_FAILURE;
    }

    EncryptionManager emgr(pm.get_password());

    // connect to back up server
    const auto channel =
        CreateChannel(config.backup_server, grpc::InsecureChannelCredentials());
    if (channel == nullptr) {
        BOOST_LOG_TRIVIAL(fatal)
            << "Cannot connect to backup server" << std::endl;
        return EXIT_FAILURE;
    }
    const auto client_stub = Backup::NewStub(channel);

    // open the storage file
    const int fd = open(config.file.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        BOOST_LOG_TRIVIAL(fatal) << "Cannot open img file" << std::endl;
        return EXIT_FAILURE;
    }
    if (config.mode == Mode::SETUP) {
        ftruncate(fd, config.n_blocks * BLOCK_SIZE);
    } else {
        config.size = lseek(fd, 0, SEEK_END);
        config.n_blocks = config.size / BLOCK_SIZE;
    }
    BOOST_LOG_TRIVIAL(info)
        << "Storage file opened, size: " << config.size << std::endl;

    // initialize
    if (config.mode == Mode::SETUP) {
        BOOST_LOG_TRIVIAL(info)
            << "Setting up SeCloud, size: " << config.size << std::endl;
        if (!utils::rebuild_remote(fd, emgr, client_stub, config)) {
            BOOST_LOG_TRIVIAL(fatal) << "Failed to setup remote" << std::endl;
            return EXIT_FAILURE;
        }
    } else if (config.check &&
               !utils::consistency_check(fd, emgr, client_stub, config) &&
               config.mode == Mode::NORMAL) {
        BOOST_LOG_TRIVIAL(fatal)
            << "Storage file is not consistent. Please specify recovery "
               "mode(rebuild_backup or recover_local)"
            << std::endl;
        exit(EXIT_FAILURE);
    } else if (config.mode == Mode::REBUILD_BACKUP) {
        if (!utils::rebuild_remote(fd, emgr, client_stub, config)) {
            BOOST_LOG_TRIVIAL(fatal) << "Failed to rebuild remote" << std::endl;
            return EXIT_FAILURE;
        }
    } else if (config.mode == Mode::RECOVER_LOCAL) {
        if (!utils::recover_local(fd, emgr, client_stub, config)) {
            BOOST_LOG_TRIVIAL(fatal) << "Failed to recover local" << std::endl;
            return EXIT_FAILURE;
        }
    }

    // start backup daemon
    const auto queue = std::make_shared<AsyncOperationQueue>(config.queue_size);
    StopFlag stop_flag(false);
    auto daemon_fd = open(config.file.c_str(), O_RDONLY);
    if (daemon_fd < 0) {
        BOOST_LOG_TRIVIAL(fatal) << "Cannot open img file" << std::endl;
        return EXIT_FAILURE;
    }
    std::thread daemon([&] {
        BackupDaemon::start(queue, daemon_fd, emgr, client_stub, stop_flag);
    });  // start the daemon

    // configure buse
    LocalBlockDriver::Context ctx = {.queue = queue, .fd = fd};
    const buse_operations bop = {
        .read = LocalBlockDriver::read,
        .write = LocalBlockDriver::write,
        .disc = LocalBlockDriver::disc,
        .flush = LocalBlockDriver::flush,
        .trim = LocalBlockDriver::trim,
        .blksize = BLOCK_SIZE,
        .size_blocks = config.n_blocks,
    };

    BOOST_LOG_TRIVIAL(info) << "SeCloud starts!" << std::endl;
    if (buse_main(BLOCK_DEV, &bop, &ctx) != EXIT_SUCCESS) {
        BOOST_LOG_TRIVIAL(fatal) << "Buse returns error" << std::endl;
    } else {
        BOOST_LOG_TRIVIAL(info) << "Buse exits normally" << std::endl;
    }

    stop_flag.store(true);
    daemon.join();
}
