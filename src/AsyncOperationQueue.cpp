#include "AsyncOperationQueue.h"

#include <boost/log/trivial.hpp>

void AsyncOperationQueue::push(const std::shared_ptr<WriteOperation>& op) {
    empty_slots.acquire();
    queue.push(op);
    filled_slots.release();
}

std::optional<std::shared_ptr<WriteOperation>> AsyncOperationQueue::pop() {
    if (!filled_slots.try_acquire_for(std::chrono::seconds(1)))
        return std::nullopt;
    if (queue.empty()) {
        BOOST_LOG_TRIVIAL(fatal) << "nothing in the queue" << std::endl;
    }
    auto op = std::move(queue.front());
    queue.pop();
    empty_slots.release();
    return op;
}