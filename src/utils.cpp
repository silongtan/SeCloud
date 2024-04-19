#include "utils.h"

#include <grpcpp/create_channel.h>

#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

#include "BackupServer.grpc.pb.h"
namespace po = boost::program_options;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

namespace utils {
bool consistency_check(int img_fd, EncryptionManager &emgr,
                       const std::unique_ptr<Backup::Stub> &client_stub,
                       const Config &config) {
    BOOST_LOG_TRIVIAL(info) << "Checking consistency" << std::endl;

    ClientContext context;
    std::shared_ptr<ClientReaderWriter<ReadBlockRequest, ReadBlockResponse>>
        stream(client_stub->ReadBlock(&context));

    for (auto block_no = 0; block_no < config.n_blocks; block_no++) {
        ReadBlockRequest req;
        ReadBlockResponse resp;

        std::array<uint8_t, BLOCK_SIZE> encrypted_buf{};
        req.set_block_no(block_no);
        if (!stream->Write(req)) {
            BOOST_LOG_TRIVIAL(error) << "RPC stream closed" << std::endl;
            return false;
        }

        if (!stream->Read(&resp)) {
            BOOST_LOG_TRIVIAL(error) << "RPC stream closed" << std::endl;
            return false;
        }

        if (!resp.success()) {
            BOOST_LOG_TRIVIAL(warning)
                << "RPC Read Block Failed: " << resp.message() << std::endl;
            return false;
        }
        std::copy(resp.data().begin(), resp.data().end(),
                  encrypted_buf.begin());

        auto decrypted_buf = emgr.decrypt_block(encrypted_buf, block_no);

        std::array<uint8_t, BLOCK_SIZE> buf{};
        if (const auto err =
                pread(img_fd, buf.data(), BLOCK_SIZE,
                      static_cast<long int>(block_no * BLOCK_SIZE));
            err < 0) {
            BOOST_LOG_TRIVIAL(error)
                << "Consistency check pread failed" << std::endl;
        }

        // compare bits
        if (buf != decrypted_buf) {
            BOOST_LOG_TRIVIAL(warning)
                << "Block " << block_no << " is not consistent" << std::endl;
            return false;
        }
    }
    stream->WritesDone();
    Status status = stream->Finish();
    if (!status.ok()) {
        BOOST_LOG_TRIVIAL(error)
            << "RPC read block stream close failed: " << status.error_message()
            << std::endl;
        return false;
    }
    return true;
}

bool rebuild_remote(int img_fd, EncryptionManager &emgr,
                    const std::unique_ptr<Backup::Stub> &client_stub,
                    const Config &config) {
    BOOST_LOG_TRIVIAL(info) << "Rebuilding remote backup" << std::endl;
    // setup
    SetupRequest setup_req;
    SetupResponse setup_resp;
    ClientContext setup_context;

    setup_req.set_size(config.size);
    if (auto status =
            client_stub->Setup(&setup_context, setup_req, &setup_resp);
        !status.ok()) {
        BOOST_LOG_TRIVIAL(error)
            << "RPC Setup Failed: " << status.error_message() << std::endl;
        return false;
    }
    if (!setup_resp.success()) {
        BOOST_LOG_TRIVIAL(error)
            << "RPC Setup is not successful: " << setup_resp.message()
            << std::endl;
        return false;
    }

    // rebuild
    WriteBlockResponse resp;
    ClientContext context;
    std::unique_ptr<ClientWriter<WriteBlockRequest>> writer(
        client_stub->WriteBlock(&context, &resp));
    for (auto block_no = 0; block_no < config.n_blocks; block_no++) {
        // report progress
        if (block_no % 10000 == 0) {
            BOOST_LOG_TRIVIAL(info)
                << boost::format("Rebuilding block %1%/%2%") % block_no %
                       config.n_blocks
                << std::endl;
        }

        WriteBlockRequest req;

        std::array<uint8_t, BLOCK_SIZE> buf{};
        if (const auto err =
                pread(img_fd, buf.data(), BLOCK_SIZE,
                      static_cast<long int>(block_no * BLOCK_SIZE));
            err < 0) {
            BOOST_LOG_TRIVIAL(error)
                << "Startup scan pread failed" << std::endl;
        }

        // encrypt block
        auto encrypted_buf = emgr.encrypt_block(buf, block_no);

        req.set_block_no(block_no);
        req.set_data(encrypted_buf.data(), BLOCK_SIZE);

        if (!writer->Write(req)) {
            BOOST_LOG_TRIVIAL(error) << "RPC stream closed" << std::endl;
            return false;
        }
    }

    writer->WritesDone();
    Status status = writer->Finish();
    if (!status.ok()) {
        BOOST_LOG_TRIVIAL(error)
            << "RPC write block stream close failed: " << status.error_message()
            << std::endl;
        return false;
    }
    if (!resp.success()) {
        BOOST_LOG_TRIVIAL(warning)
            << "RPC Write Block Failed: " << resp.message() << std::endl;
        return false;
    }
    return true;
}

bool recover_local(int img_fd, EncryptionManager &emgr,
                   const std::unique_ptr<Backup::Stub> &client_stub,
                   const Config &config) {
    BOOST_LOG_TRIVIAL(info)
        << "Recovering local disk with remote backup" << std::endl;

    ClientContext context;
    std::shared_ptr<ClientReaderWriter<ReadBlockRequest, ReadBlockResponse>>
        stream(client_stub->ReadBlock(&context));

    for (auto block_no = 0; block_no < config.n_blocks; block_no++) {
        // report progress
        if (block_no % 1000 == 0) {
            BOOST_LOG_TRIVIAL(info)
                << boost::format("Recovering local block %1%/%2%") % block_no %
                       config.n_blocks
                << std::endl;
        }

        ReadBlockRequest req;
        ReadBlockResponse resp;

        std::array<uint8_t, BLOCK_SIZE> encrypted_buf{};
        req.set_block_no(block_no);
        if (!stream->Write(req)) {
            BOOST_LOG_TRIVIAL(error) << "RPC stream closed" << std::endl;
            return false;
        }

        if (!stream->Read(&resp)) {
            BOOST_LOG_TRIVIAL(error) << "RPC stream closed" << std::endl;
            return false;
        }

        if (!resp.success()) {
            BOOST_LOG_TRIVIAL(warning)
                << "RPC Read Block Failed: " << resp.message() << std::endl;
            return false;
        }

        std::copy(resp.data().begin(), resp.data().end(),
                  encrypted_buf.begin());

        auto decrypted_buf = emgr.decrypt_block(encrypted_buf, block_no);

        if (block_no == 0)
            BOOST_LOG_TRIVIAL(debug)
                << "Recovering block " << decrypted_buf[0] << std::endl;

        if (const auto err =
                pwrite(img_fd, decrypted_buf.data(), BLOCK_SIZE,
                       static_cast<long int>(block_no * BLOCK_SIZE));
            err < 0) {
            BOOST_LOG_TRIVIAL(error) << "Recovery: pwrite failed" << std::endl;
            return false;
        }
    }
    stream->WritesDone();
    Status status = stream->Finish();
    if (!status.ok()) {
        BOOST_LOG_TRIVIAL(error)
            << "RPC read block stream close failed: " << status.error_message()
            << std::endl;
        return false;
    }
    return true;
}

Config parse_options(int argc, char *argv[]) {
    po::options_description desc("Allowed options");

    std::string mode;
    desc.add_options()("help", "produce help message");
    desc.add_options()(
        "mode",
        po::value<std::string>(&mode)->notifier([](const std::string &value) {
            std::vector<std::string> allowed_modes = {"recover_local", "setup",
                                                      "rebuild_backup"};
            if (std::find(allowed_modes.begin(), allowed_modes.end(), value) ==
                allowed_modes.end()) {
                throw po::validation_error(
                    po::validation_error::invalid_option_value);
            }
        }),
        "set mode: recover_local | rebuild_backup | setup\n"
        "recover_local: overwrite the local disk blocks with remote back up\n"
        "rebuild_backup: rebuild the remote backup image with local disk\n"
        "setup: setup the remote backup image with local disk\n");
    desc.add_options()("check", "force check consistency of the storage file");
    desc.add_options()("file", po::value<std::string>(), "storage file path");
    desc.add_options()("size", po::value<uint64_t>(),
                       "storage file size(in MB)");
    desc.add_options()("queue_size", po::value<size_t>(), "queue size");
    desc.add_options()("v", "verbose");
    desc.add_options()("backup_server", po::value<std::string>(),
                       "backup server address");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(0);
    }

    if (vm.count("mode")) {
        mode = vm["mode"].as<std::string>();
    }

    Config config;
    if (mode == "recover_local") {
        config.mode = Mode::RECOVER_LOCAL;
    } else if (mode == "rebuild_backup") {
        config.mode = Mode::REBUILD_BACKUP;
    } else if (mode == "setup") {
        config.mode = Mode::SETUP;
    } else {
        config.mode = Mode::NORMAL;
    }
    if (vm.count("check")) {
        config.check = true;
    }
    if (vm.count("file")) {
        config.file = vm["file"].as<std::string>();
    }
    if (vm.count("size")) {
        if (config.mode != Mode::SETUP) {
            throw std::invalid_argument(
                "size cannot be specified in non-setup mode");
        }
        config.size = vm["size"].as<uint64_t>() * 1024 * 1024;
        if (config.size % BLOCK_SIZE != 0) {
            throw std::invalid_argument("size must be multiple of block size");
        }
        config.n_blocks = config.size / BLOCK_SIZE;
    }
    if (vm.count("queue_size")) {
        config.queue_size = vm["queue_size"].as<size_t>();
    }
    if (vm.count("v")) {
        config.verbose = true;
    }
    if (vm.count("backup_server")) {
        config.backup_server = vm["backup_server"].as<std::string>();
    }
    return config;
}
}  // namespace utils