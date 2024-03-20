#ifndef DAEMON_H
#define DAEMON_H

#include <boost/lockfree/spsc_queue.hpp>

#include "consts.h"
#include "EncryptionManager.h"
#include "types.h"

class Daemon {
   public:
    static void start(OperationQueue& spsc, EncryptionManager& emgr,
                      StopFlag& stop);
};

#endif