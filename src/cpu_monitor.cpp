#include "cpu_monitor.h"

#include <windows.h>
#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

static inline const auto ft2int(const FILETIME &ftime) {
    LARGE_INTEGER li{};
    li.LowPart  = ftime.dwLowDateTime;
    li.HighPart = ftime.dwHighDateTime;
    return li.QuadPart;
}

CpuMonitor::CpuMonitor()
    : worker_(&CpuMonitor::workerProcess, this)
    , nextListenerId_{0} {
    worker_.detach();
}

int CpuMonitor::addListener(slot_t listener) {
    std::lock_guard guard(listenerMutex_);
    const auto      id = nextListenerId_++;
    listenerList_.insert({id, listener});
    return id;
}

void CpuMonitor::removeListener(int listenerId) {
    std::lock_guard guard(listenerMutex_);
    listenerList_.erase(listenerId);
}

void CpuMonitor::workerProcess() const {
    while (true) {
        FILETIME idle[2]{};
        FILETIME kernel[2]{};
        FILETIME user[2]{};
        GetSystemTimes(&idle[0], &kernel[0], &user[0]);

        std::this_thread::sleep_for(1s);

        GetSystemTimes(&idle[1], &kernel[1], &user[1]);

        const auto idleDiff   = ft2int(idle[1]) - ft2int(idle[0]);
        const auto kernelDiff = ft2int(kernel[1]) - ft2int(kernel[0]);
        const auto userDiff   = ft2int(user[1]) - ft2int(user[0]);

        const auto busy     = userDiff + kernelDiff;
        const auto cpuUsage = (busy - idleDiff) * 1.0 / (busy);

        std::lock_guard guard(listenerMutex_);
        for (auto &[listenerId, resolve] : listenerList_) {
            //! FIXME: handle blocking issue
            resolve(cpuUsage);
        }
    }
}
