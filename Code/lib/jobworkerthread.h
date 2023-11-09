// This thing will RUN job
#pragma once
#include <mutex>
#include <map>
#include <deque>
#include <vector>
#include <thread>

#include "job.h"

class JobSystem; // pointer... its is a forward class declaration... promise to the compiler... that it will find the definition to this object
// the compiler will trust you... if it don't find it we get a linker error.

class JobWorkerThread
{
    friend class JobSystem;

private:
    JobWorkerThread(const char *uniqueName, unsigned long workerJobChannels, JobSystem *jobSystem);
    ~JobWorkerThread();

    void StartUp();
    void Work();     // Called in private thread, blocks forever until StopWorking() is called
    void ShutDown(); // Signal that work should at next opportunity

    bool isStopping() const;
    void SetWorkerJobChannels(unsigned long workerJobChannels);
    static void WorkerThreadMain(void *workThreadObject);

private:
    const char *m_uniqueName;
    unsigned long m_workerJobChannels = 0xffffffff;
    bool m_isStopping = false;
    JobSystem *m_jobSystem = nullptr;
    std::thread *m_thread = nullptr;
    mutable std::mutex m_workerStatusMutex;
};