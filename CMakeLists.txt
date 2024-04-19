cmake_minimum_required(VERSION 3.0.0)
project(SeCloud VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)

# google test
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(googletest)

# boost
find_package(Boost REQUIRED COMPONENTS log log_setup program_options)
message(STATUS "Using Boost ${Boost_VERSION}")
include_directories(${Boost_INCLUDE_DIRS})

# absl
set(ABSL_ENABLE_INSTALL ON)
FetchContent_Declare(
        absl
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG 20240116.1
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(absl)
message(STATUS "Using Abseil ${absl_VERSION}")

# grpc
FetchContent_Declare(
        gRPC
        GIT_REPOSITORY https://github.com/grpc/grpc
        GIT_TAG v1.62.1
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(gRPC)

set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP grpc++)
if (CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else ()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
endif ()

# protobuf
find_package(Protobuf REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")
get_filename_component(proto "src/BackupServer.proto" ABSOLUTE)
get_filename_component(proto_path "${proto}" PATH)

set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/BackupServer.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/BackupServer.pb.h")
set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/BackupServer.grpc.pb.cc")
set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/BackupServer.grpc.pb.h")
add_custom_command(
        OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${proto}"
        DEPENDS "${proto}")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(grpc_proto
        ${grpc_srcs}
        ${grpc_hdrs}
        ${proto_srcs}
        ${proto_hdrs})
target_link_libraries(grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

# openssl
find_package(OpenSSL REQUIRED)
message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
include_directories(${OPENSSL_INCLUDE_DIR})

# SeCloud
add_executable(SeCloud
        src/SeCloud.cpp
        src/BUSE/buse.c src/BUSE/buse.h
        src/AsyncOperationQueue.h src/AsyncOperationQueue.cpp
        src/BackupDaemon.h src/BackupDaemon.cpp
        src/EncryptionManager.h src/EncryptionManager.cpp
        src/LocalBlockDriver.h src/LocalBlockDriver.cpp
        src/utils.h src/utils.cpp
        src/PasswordManager.h src/PasswordManager.cpp
        src/types.h
)
target_link_libraries(SeCloud
        Boost::log Boost::log_setup
        Boost::program_options
        grpc_proto
        absl::flags
        absl::flags_parse
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
        ${OPENSSL_LIBRARIES}
)

# Backup Server
add_executable(BackupServer src/BackupServer.cpp)
target_link_libraries(BackupServer
        Boost::log Boost::log_setup
        Boost::program_options
        grpc_proto
        absl::flags
        absl::flags_parse
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
        ${OPENSSL_LIBRARIES}
)

# testings
enable_testing()
add_executable(tests
        tests/EncryptionManagerTest.cpp
        src/BUSE/buse.c src/BUSE/buse.h
        src/AsyncOperationQueue.h src/AsyncOperationQueue.cpp
        src/BackupDaemon.h src/BackupDaemon.cpp
        src/EncryptionManager.h src/EncryptionManager.cpp
        src/LocalBlockDriver.h src/LocalBlockDriver.cpp
        src/utils.h src/utils.cpp
        src/PasswordManager.h src/PasswordManager.cpp
)
target_link_libraries(tests
        gtest_main
        Boost::log Boost::log_setup
        Boost::program_options
        grpc_proto
        absl::flags
        absl::flags_parse
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
        ${OPENSSL_LIBRARIES}
)
include(GoogleTest)
gtest_discover_tests(tests)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})