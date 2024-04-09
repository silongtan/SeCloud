#include "utils.h"

#include <grpcpp/create_channel.h>

#include <boost/format.hpp>
#include <boost/log/trivial.hpp>

#include "BackupServer.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

namespace utils {
bool consistency_check(int img_fd, EncryptionManager &emgr,
                       const std::unique_ptr<Backup::Stub> &client_stub) {
    for (auto block_no = 0; block_no < N_BLOCKS; block_no++) {
        ReadBlockRequest req;
        ReadBlockResponse resp;
        ClientContext context;

        std::array<uint8_t, BLOCK_SIZE> encrypted_buf{};
        req.set_block_no(block_no);
        if (const auto status = client_stub->ReadBlock(&context, req, &resp);
            !status.ok()) {
            BOOST_LOG_TRIVIAL(error)
                << "RPC Read Block Failed: " << status.error_message()
                << std::endl;
            return false;
        } else {
            if (!resp.success()) {
                BOOST_LOG_TRIVIAL(warning)
                    << "RPC Read Block Failed: " << resp.message() << std::endl;
                return false;
            }
            std::copy(resp.data().begin(), resp.data().end(),
                      encrypted_buf.begin());
        }

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
    return true;
}

bool rebuild_remote(int img_fd, EncryptionManager &emgr,
                    const std::unique_ptr<Backup::Stub> &client_stub) {
    for (auto block_no = 0; block_no < N_BLOCKS; block_no++) {
        WriteBlockRequest req;
        WriteBlockResponse resp;
        ClientContext context;

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

        if (const auto status = client_stub->WriteBlock(&context, req, &resp);
            !status.ok()) {
            BOOST_LOG_TRIVIAL(error)
                << "RPC Write Block Failed: " << status.error_message()
                << std::endl;
            return false;
        } else if (!resp.success()) {
            BOOST_LOG_TRIVIAL(warning)
                << "RPC Write Block Failed: " << resp.message() << std::endl;
            return false;
        }
    }
    return true;
}

bool recover_local(int img_fd, EncryptionManager &emgr,
                   const std::unique_ptr<Backup::Stub> &client_stub) {
    for (auto block_no = 0; block_no < N_BLOCKS; block_no++) {
        ReadBlockRequest req;
        ReadBlockResponse resp;
        ClientContext context;

        std::array<uint8_t, BLOCK_SIZE> encrypted_buf{};
        req.set_block_no(block_no);
        if (const auto status = client_stub->ReadBlock(&context, req, &resp);
            !status.ok()) {
            BOOST_LOG_TRIVIAL(error)
                << "RPC Read Block Failed: " << status.error_message()
                << std::endl;
            return false;
        } else {
            if (!resp.success()) {
                BOOST_LOG_TRIVIAL(warning)
                    << "RPC Read Block Failed: " << resp.message() << std::endl;
                return false;
            }
            std::copy(resp.data().begin(), resp.data().end(),
                      encrypted_buf.begin());
        }

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
    return true;
}
}  // namespace utils