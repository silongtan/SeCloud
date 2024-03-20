#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

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

#define BOOST_LOG_DYN_LINK 1

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class BackupServiceImpl final : public Backup::Service {
    // Sends a block of data to be written.
    Status WriteBlock(ServerContext* context,
                              const WriteBlockRequest* request,
                              WriteBlockResponse* response) override;
    // Reads a block of data.
    Status ReadBlock(ServerContext* context,
                             const ReadBlockRequest* request,
                             ReadBlockResponse* response) override;
};

int main(int, char**) {
    boost::log::add_console_log(std::cout,
                                boost::log::keywords::format = ">> %Message%");

    BOOST_LOG_TRIVIAL(info) << "SeCloud backup server starts!" << std::endl;
    std::string server_address = absl::StrFormat("0.0.0.0:%d", 8080);
    BackupServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

Status BackupServiceImpl::WriteBlock(ServerContext* context,
                                     const WriteBlockRequest* request,
                                     WriteBlockResponse* response) {
    // TODO
    return Status();
}

Status BackupServiceImpl::ReadBlock(ServerContext* context,
                                    const ReadBlockRequest* request,
                                    ReadBlockResponse* response) {
    // TODO
    return Status();
}
