#include "LocalBlockDriver.h"

#include <boost/format.hpp>
#include <boost/log/trivial.hpp>

namespace LocalBlockDriver {

int read(void *buf, const uint32_t len, const uint64_t offset, void *userdata) {
    BOOST_LOG_TRIVIAL(debug)
        << "Read block len: " << len << ", offset: " << offset << std::endl;

    const auto ctx = static_cast<Context *>(userdata);

    if (const auto bytes_read =
            pread(ctx->fd, buf, len, static_cast<long>(offset));
        bytes_read < 0) {
        BOOST_LOG_TRIVIAL(error) << "Read failed" << std::endl;
    } else {
        BOOST_LOG_TRIVIAL(debug)
            << boost::format("Read %1% bytes") % bytes_read << std::endl;
    }

    return 0;
}

int write(const void *buf, const uint32_t len, uint64_t offset,
          void *userdata) {
    BOOST_LOG_TRIVIAL(debug)
        << "Write block len: " << len << ", offset: " << offset << std::endl;

    const auto ctx = static_cast<Context *>(userdata);

    if (const auto bytes_write =
            pwrite(ctx->fd, buf, len, static_cast<long>(offset));
        bytes_write < 0) {
        BOOST_LOG_TRIVIAL(error) << "Write failed" << std::endl;
    } else {
        BOOST_LOG_TRIVIAL(debug) << "Write success" << std::endl;
    }

    uint64_t block_no_start = offset / BLOCK_SIZE;
    uint64_t block_no_end = (offset + len - 1) / BLOCK_SIZE;

    ctx->queue->push(
        std::make_shared<WriteOperation>(block_no_start, block_no_end));
    return 0;
}

int flush(void *userdata) {
    BOOST_LOG_TRIVIAL(debug) << "Flush" << std::endl;

    //    const auto ctx = static_cast<Context *>(userdata);
    //    fsync(ctx->fd);

    return 0;
}

void disc(void *userdata) {
    BOOST_LOG_TRIVIAL(debug) << "Disconnect" << std::endl;
    const auto ctx = static_cast<Context *>(userdata);

    close(ctx->fd);
}

int trim(const uint64_t from, const uint32_t len, void *userdata) {
    BOOST_LOG_TRIVIAL(debug)
        << "Trim block len: " << len << ", from: " << from << std::endl;
    // TODO
    return 0;
}

}  // namespace LocalBlockDriver
