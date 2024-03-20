#ifndef TYPES_H
#define TYPES_H

#include <boost/lockfree/spsc_queue.hpp>

#include "consts.h"

typedef boost::lockfree::spsc_queue<int, boost::lockfree::capacity<SPSC_SIZE>>
    OperationQueue;

typedef std::atomic<bool> StopFlag;

#endif