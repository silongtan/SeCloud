#include <fcntl.h>
#include <grpcpp/create_channel.h>

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <thread>

#include "BUSE/buse.h"
#include "BackupDaemon.h"
#include "BackupServer.grpc.pb.h"
#include "EncryptionManager.h"
#include "LocalBlockDriver.h"
#include "utils.h"

#define BOOST_LOG_DYN_LINK 1

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
namespace po = boost::program_options;

po::options_description setup_po() {
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
        "mode",
        po::value<std::string>()->notifier([](const std::string& value) {
            std::vector<std::string> allowed_modes = {
                "setup", "recover_local", "rebuild_remote", "normal"};
            if (std::find(allowed_modes.begin(), allowed_modes.end(), value) ==
                allowed_modes.end()) {
                throw po::validation_error(
                    po::validation_error::invalid_option_value);
            }
        }),
        "set mode: setup | recover_local | rebuild_remote | normal\nsetup: "
        "setup the storage\nrecover_local: overwrite the local disk blocks "
        "with remote back up\nrebuild_remote: rebuild the remote backup "
        "image with local disk blocks");
    return desc;
}

enum OperationMode { Normal, RebuildRemote, RecoverLocal };

int main(int argc, char* argv[]) {
    boost::log::add_console_log(std::cout,
                                boost::log::keywords::format = ">> %Message%");

    auto desc = setup_po();

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
    po::notify(vm);

    OperationMode mode = OperationMode::Normal;
    if (vm.count("help")) {
        std::cout << desc << "\n";
        return EXIT_SUCCESS;
    } else if (vm.count("mode")) {
        auto mode_str = vm["mode"].as<std::string>();
        if (mode_str == "recover_local") {
            mode = OperationMode::RecoverLocal;
        } else if (mode_str == "rebuild_remote" || mode_str == "setup") {
            mode = OperationMode::RebuildRemote;
        }
    }

    const auto queue =
        std::make_shared<AsyncOperationQueue>();  // queue for communicating
                                                  // between daemon and buse
    std::string key_str = "StoreDuke566!";
    std::vector<uint8_t> key(key_str.begin(), key_str.end());
    std::array<uint8_t, USER_IV_SIZE> iv = {};
    EncryptionManager emgr(key, iv);
    StopFlag stop_flag(false);

    BOOST_LOG_TRIVIAL(info) << "SeCloud starts!" << std::endl;

    // connect to back up server
    const auto channel =
        CreateChannel(BACKUP_SERVER_ADDR, grpc::InsecureChannelCredentials());
    const auto client_stub = Backup::NewStub(channel);

    // open the storage file
    const int fd = open(IMG_FILE, O_RDWR);
    if (fd < 0) {
        BOOST_LOG_TRIVIAL(fatal) << "Cannot open img file" << std::endl;
        return EXIT_FAILURE;
    }
    LocalBlockDriver::Context ctx = {.queue = queue, .fd = fd};

    if (!utils::consistency_check(fd, emgr, client_stub)) {
        BOOST_LOG_TRIVIAL(warning)
            << "Storage file is not consistent, doing recover..." << std::endl;
        if (mode == OperationMode::RecoverLocal) {
            if (!utils::recover_local(fd, emgr, client_stub)) {
                BOOST_LOG_TRIVIAL(fatal)
                    << "Cannot recover storage file" << std::endl;
                return EXIT_FAILURE;
            }
        } else if (mode == OperationMode::RebuildRemote) {
            if (!utils::rebuild_remote(fd, emgr, client_stub)) {
                BOOST_LOG_TRIVIAL(fatal)
                    << "Cannot recover storage file" << std::endl;
                return EXIT_FAILURE;
            };
        } else {
            BOOST_LOG_TRIVIAL(fatal)
                << "Inconsistency found between local and remote backup"
                << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::thread daemon([&] {
        BackupDaemon::start(queue, fd, emgr, client_stub, stop_flag);
    });  // start the daemon

    constexpr buse_operations bop = {
        .read = LocalBlockDriver::read,
        .write = LocalBlockDriver::write,
        .disc = LocalBlockDriver::disc,
        .flush = LocalBlockDriver::flush,
        .trim = LocalBlockDriver::trim,
        .blksize = BLOCK_SIZE,
        .size_blocks = N_BLOCKS,
    };
    if (buse_main(BLOCK_DEV, &bop, &ctx) != EXIT_SUCCESS) {
        BOOST_LOG_TRIVIAL(fatal) << "Buse returns error" << std::endl;
    } else {
        BOOST_LOG_TRIVIAL(info) << "Buse exits normally" << std::endl;
    }

    stop_flag.store(true);
    daemon.join();
}
