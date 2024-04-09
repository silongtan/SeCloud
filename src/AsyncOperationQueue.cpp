//
// Created by 陈天予 on 2024/3/24.
//

#include "AsyncOperationQueue.h"

#include <boost/log/trivial.hpp>

void AsyncOperationQueue::push(std::unique_ptr<WriteOperation> op) {
    queue.push(std::move(op));
    size.release();
}

std::optional<std::unique_ptr<WriteOperation>> AsyncOperationQueue::pop() {
    if (!size.try_acquire_for(std::chrono::seconds(1))) return std::nullopt;
    if (queue.empty()) {
        BOOST_LOG_TRIVIAL(fatal) << "nothing in the queue" << std::endl;
    }
    auto op = std::move(queue.front());
    queue.pop();
    return op;
}