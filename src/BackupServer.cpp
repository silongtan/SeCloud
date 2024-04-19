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

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::Status;

class BackupServiceImpl final : public Backup::Service {
    int encrypted_fd = -1;
    const char* filepath;

   public:
    explicit BackupServiceImpl(const char* filepath) : filepath(filepath) {
        encrypted_fd = open(filepath, O_RDWR);
        if (encrypted_fd == -1) {
            BOOST_LOG_TRIVIAL(info)
                << "File not setup, waiting for setup request" << std::endl;
        }
    }

    Status Setup(ServerContext* context, const SetupRequest* request,
                 SetupResponse* response) override;

    Status WriteBlock(ServerContext* context,
                      ServerReader<WriteBlockRequest>* reader,
                      WriteBlockResponse* response) override;

    Status ReadBlock(ServerContext* context,
                     ServerReaderWriter<ReadBlockResponse, ReadBlockRequest>*
                         stream) override;
};

void usage() {
    std::cout << "Usage: BackupServer [-v] [file]" << std::endl;
    exit(1);
}

int main(int argc, char** argv) {
    boost::log::add_console_log(std::cout,
                                boost::log::keywords::format = ">> %Message%");

    // extract path from argument
    const char* file = nullptr;
    if (argc > 3) {
        usage();
        exit(EXIT_FAILURE);
    } else if (argc == 3) {
        file = argv[2];
    } else if (argc == 2) {
        boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                            boost::log::trivial::info);
        file = argv[1];
    } else {
        boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                            boost::log::trivial::info);
        file = ENCRYPTED_IMG;
    }

    BOOST_LOG_TRIVIAL(info) << "SeCloud backup server starts!" << std::endl;
    const std::string server_address = absl::StrFormat("0.0.0.0:%d", 8080);
    BackupServiceImpl service(file);

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    const std::unique_ptr server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

Status BackupServiceImpl::Setup(ServerContext* context,
                                const SetupRequest* request,
                                ::SetupResponse* response) {
    BOOST_LOG_TRIVIAL(info)
        << "Setting up file, size: " << request->size() << std::endl;

    encrypted_fd = open(filepath, O_RDWR | O_CREAT, 0666);
    if (encrypted_fd == -1) {
        BOOST_LOG_TRIVIAL(fatal)
            << "Cannot open encrypted backup img" << std::endl;
        response->set_success(false);
        response->set_message("Cannot open encrypted backup img");
        return grpc::Status::OK;
    }
    if (0 != ftruncate(encrypted_fd, request->size())) {
        BOOST_LOG_TRIVIAL(fatal)
            << "Cannot truncate encrypted backup img" << std::endl;
        response->set_success(false);
        response->set_message("Cannot truncate encrypted backup img");
        return grpc::Status::OK;
    }

    response->set_success(true);
    return grpc::Status::OK;
}

Status BackupServiceImpl::WriteBlock(ServerContext* context,
                                     ServerReader<WriteBlockRequest>* reader,
                                     WriteBlockResponse* response) {
    if (encrypted_fd == -1) {
        BOOST_LOG_TRIVIAL(fatal) << "File not setup" << std::endl;
        response->set_success(false);
        response->set_message("File not setup");
        return Status::OK;
    }

    WriteBlockRequest request;
    while (reader->Read(&request)) {
        BOOST_LOG_TRIVIAL(debug)
            << "Writing block " << request.block_no()
            << " with data size: " << request.data().size() << std::endl;

        const auto offset = request.block_no() * BLOCK_SIZE;
        if (const auto bytes_write =
                pwrite(encrypted_fd, request.data().data(),
                       request.data().size(), static_cast<long>(offset));
            bytes_write < 0) {
            BOOST_LOG_TRIVIAL(error) << "Write failed" << std::endl;
            response->set_success(false);
            response->set_message("Write failed");
            return Status::OK;
        }

        BOOST_LOG_TRIVIAL(debug) << "Write succeeded" << std::endl;
    }

    response->set_success(true);
    response->set_message("Block written successfully.");
    return Status::OK;
}

Status BackupServiceImpl::ReadBlock(
    ServerContext* context,
    ServerReaderWriter<ReadBlockResponse, ReadBlockRequest>* stream) {
    ReadBlockRequest request;
    ReadBlockResponse response;

    if (encrypted_fd == -1) {
        BOOST_LOG_TRIVIAL(fatal) << "File not setup" << std::endl;
        response.set_success(false);
        response.set_message("File not setup");
        stream->Write(response);
        return Status::OK;
    }

    while (stream->Read(&request)) {
        BOOST_LOG_TRIVIAL(debug)
            << "Reading block " << request.block_no() << std::endl;

        auto offset = request.block_no() * BLOCK_SIZE;
        std::vector<char> buffer(BLOCK_SIZE);

        if (const auto bytes_read =
                pread(encrypted_fd, buffer.data(), BLOCK_SIZE,
                      static_cast<long>(offset));
            bytes_read < 0) {
            BOOST_LOG_TRIVIAL(error) << "Read failed" << std::endl;
            response.set_success(false);
            response.set_message("Read failed");
            stream->Write(response);
            return Status::OK;
        }

        response.set_data(buffer.data(), BLOCK_SIZE);
        response.set_success(true);
        stream->Write(response);
    }

    return Status::OK;
}
