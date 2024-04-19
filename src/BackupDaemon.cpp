#include "BackupDaemon.h"

#include <grpcpp/create_channel.h>

#include <boost/format.hpp>
#include <boost/log/trivial.hpp>
#include <thread>

#include "AsyncOperationQueue.h"
#include "BackupServer.grpc.pb.h"

using grpc::ClientContext;
using grpc::ClientWriter;

void BackupDaemon::start(const std::shared_ptr<AsyncOperationQueue> &queue,
                         const int img_fd, EncryptionManager &emgr,
                         const std::unique_ptr<Backup::Stub> &client_stub,
                         const StopFlag &stop) {
    BOOST_LOG_TRIVIAL(info) << "Daemon starts!" << std::endl;

    WriteBlockResponse resp;
    ClientContext context;
    std::unique_ptr<ClientWriter<WriteBlockRequest>> writer(
        client_stub->WriteBlock(&context, &resp));

    while (!stop.load()) {
        const auto op = queue->pop().value_or(nullptr);
        if (!op) {
            continue;
        }

        BOOST_LOG_TRIVIAL(debug)
            << boost::format(
                   "Daemon recvs write operation, "
                   "block_no_start: %1%, block_no_end: %2%") %
                   op->block_no_start % op->block_no_end
            << std::endl;

        for (auto block_no = op->block_no_start; block_no <= op->block_no_end;
             block_no++) {
            WriteBlockRequest req;
            req.set_block_no(block_no);
            std::array<uint8_t, BLOCK_SIZE> buf{};
            if (const auto err =
                    pread(img_fd, buf.data(), BLOCK_SIZE,
                          static_cast<long int>(block_no * BLOCK_SIZE));
                err < 0) {
                BOOST_LOG_TRIVIAL(error) << "Daemon pread failed" << std::endl;
                continue;
            }

            // encrypt block
            auto encrypted_buf = emgr.encrypt_block(buf, block_no);

            req.set_data(encrypted_buf.data(), BLOCK_SIZE);

            if (!writer->Write(req)) {
                BOOST_LOG_TRIVIAL(error)
                    << "Daemon RPC stream closed, failed to write" << std::endl;
                continue;
            }
        }
    }
    close(img_fd);
}
