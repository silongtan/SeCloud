#ifndef ASYNC_OPERATION_QUEUE_H
#define ASYNC_OPERATION_QUEUE_H
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

typedef std::queue<std::unique_ptr<WriteOperation>> OperationQueue;

class AsyncOperationQueue {
    OperationQueue queue{};
    std::counting_semaphore<SPSC_SIZE> size;

   public:
    AsyncOperationQueue() : size(0) {}
    void push(std::unique_ptr<WriteOperation> op);
    std::optional<std::unique_ptr<WriteOperation>> pop();
};

#endif
