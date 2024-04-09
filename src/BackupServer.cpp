#include <fcntl.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <unistd.h>

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <memory>

#include "BackupServer.grpc.pb.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "consts.h"

#define BOOST_LOG_DYN_LINK 1

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class BackupServiceImpl final : public Backup::Service {
    int encrypted_fd;

   public:
    BackupServiceImpl() {
        encrypted_fd = open(ENCRYPTED_IMG, O_RDWR);
        if (encrypted_fd == -1) {
            BOOST_LOG_TRIVIAL(fatal)
                << "Cannot open encrypted backup img" << std::endl;
            exit(1);
        }
    }
    Status WriteBlock(ServerContext* context, const WriteBlockRequest* request,
                      WriteBlockResponse* response) override;

    Status ReadBlock(ServerContext* context, const ReadBlockRequest* request,
                     ReadBlockResponse* response) override;
};

int main(int, char**) {
    boost::log::add_console_log(std::cout,
                                boost::log::keywords::format = ">> %Message%");

    BOOST_LOG_TRIVIAL(info) << "SeCloud backup server starts!" << std::endl;
    const std::string server_address = absl::StrFormat("0.0.0.0:%d", 8080);
    BackupServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    const std::unique_ptr server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

Status BackupServiceImpl::WriteBlock(ServerContext* context,
                                     const WriteBlockRequest* request,
                                     WriteBlockResponse* response) {
    BOOST_LOG_TRIVIAL(info)
        << "Writing block " << request->block_no()
        << " with data size: " << request->data().size() << std::endl;

    const auto offset = request->block_no() * BLOCK_SIZE;
    if (const auto bytes_write =
            pwrite(encrypted_fd, request->data().data(), request->data().size(),
                   static_cast<long>(offset));
        bytes_write < 0) {
        BOOST_LOG_TRIVIAL(error) << "Write failed" << std::endl;
    } else {
        BOOST_LOG_TRIVIAL(debug) << "Write success" << std::endl;
    }

    response->set_success(true);
    response->set_message("Block written successfully.");
    return grpc::Status::OK;
}

Status BackupServiceImpl::ReadBlock(ServerContext* context,
                                    const ReadBlockRequest* request,
                                    ReadBlockResponse* response) {
    auto offset = request->block_no() * BLOCK_SIZE;

    std::vector<char> buffer(BLOCK_SIZE);

    if (const auto bytes_read = pread(encrypted_fd, buffer.data(), BLOCK_SIZE,
                                      static_cast<long>(offset));
        bytes_read < 0) {
        BOOST_LOG_TRIVIAL(error) << "Read failed" << std::endl;
    } else {
        BOOST_LOG_TRIVIAL(debug) << "Read success" << std::endl;
    }

    response->set_data(buffer.data(), BLOCK_SIZE);
    response->set_success(true);
    response->set_message("Block read successfully.");

    BOOST_LOG_TRIVIAL(info) << "Read block " << request->block_no()
                            << " successfully." << std::endl;

    return grpc::Status::OK;
}
