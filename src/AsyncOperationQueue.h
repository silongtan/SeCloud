#ifndef ASYNC_OPERATION_QUEUE_H
#define ASYNC_OPERATION_QUEUE_H
#include <boost/lockfree/spsc_queue.hpp>
#include <memory>
#include <optional>
#include <queue>
#include <semaphore>
#include <vector>

#include "consts.h"

struct WriteOperation {
    uint64_t block_no_start;
    uint64_t block_no_end;
};

typedef boost::lockfree::spsc_queue<std::shared_ptr<WriteOperation>>
    OperationQueue;

class AsyncOperationQueue {
    OperationQueue queue;
    std::counting_semaphore<SPSC_SIZE> filled_slots;
    std::counting_semaphore<SPSC_SIZE> empty_slots;
    size_t cap;

   public:
    explicit AsyncOperationQueue(size_t size)
        : filled_slots(0), empty_slots((long)size), queue(size), cap(size) {}
    void push(const std::shared_ptr<WriteOperation>& op);
    std::optional<std::shared_ptr<WriteOperation>> pop();
};

#endif
