#pragma once

#include <map>
#include <functional>
#include <thread>
#include <mutex>

class CpuMonitor {
public:
    using slot_t = std::function<void(float)>;

    static inline CpuMonitor& getInstance() {
        static Guard g;
        if (instance_ == nullptr) { instance_ = new CpuMonitor(); }
        return *instance_;
    }

    int  addListener(slot_t listener);
    void removeListener(int listenerId);

    CpuMonitor(const CpuMonitor&)            = delete;
    CpuMonitor& operator=(const CpuMonitor&) = delete;

protected:
    CpuMonitor();
    inline virtual ~CpuMonitor() = default;

    void workerProcess() const;

private:
    static inline CpuMonitor* instance_ = nullptr;
    mutable std::mutex        listenerMutex_;
    std::map<int, slot_t>     listenerList_;
    int                       nextListenerId_;
    std::thread               worker_;

    struct Guard {
        inline ~Guard() {
            delete instance_;
            instance_ = nullptr;
        }
    };
};
