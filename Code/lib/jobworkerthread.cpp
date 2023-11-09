#include "jobworkerthread.h"
#include "jobsystem.h"

JobWorkerThread::JobWorkerThread(const char *uniqueName, unsigned long workerJobChannels, JobSystem *jobSystem):
    m_uniqueName(uniqueName),
    m_workerJobChannels(workerJobChannels),
    m_jobSystem(jobSystem) {}



JobWorkerThread::~JobWorkerThread(){
    // If we havent signal
    //      Thread that we should exit as soon as it can 
    ShutDown();

    // Block on the thread's main until it has a change to finish its current job and exit
    m_thread->join();
    delete m_thread;
    m_thread = nullptr;
}

void JobWorkerThread::StartUp(){
    m_thread = new std::thread(WorkerThreadMain, this);
}

void JobWorkerThread::Work(){
    while(!isStopping()){
        m_workerStatusMutex.lock();
        unsigned long workerJobChannels = m_workerJobChannels;
        m_workerStatusMutex.unlock();

        Job* job = m_jobSystem->ClaimAJob(m_workerJobChannels); //this thread wants to get a job... given the channels. If there is a job with compatible channels, the thread get it
        if(job){ // IF we get a thread
            job->Execute();
            m_jobSystem->OnJobCompleted(job); // Update the status of this job. the job is moved from running queue to completed queue. Call the job system to perform this move.
        }

        std::this_thread::sleep_for( std::chrono::microseconds(1) ); // Rest a little bit. To reliquishing control a little bit to the CPU. We don't want to poll jobs to quickly as well.
    }
}

void JobWorkerThread::ShutDown(){
    m_workerStatusMutex.lock();
    m_isStopping = true;
    m_workerStatusMutex.unlock();
}


bool JobWorkerThread::isStopping() const {
    m_workerStatusMutex.lock();
    bool shouldClose = m_isStopping; // You could have returned directly, but it'd be bad practice. The variable can be read at anytime... by something else. You don't want to pass it directly. instead pass/return a copy of them
    m_workerStatusMutex.unlock();

    return shouldClose;
}

void JobWorkerThread::SetWorkerJobChannels(unsigned long workerJobChannels){
    m_workerStatusMutex.lock();
    m_workerJobChannels = workerJobChannels;
    m_workerStatusMutex.unlock();
}

void JobWorkerThread::WorkerThreadMain(void* workThreadObject){
    JobWorkerThread* thisWorker = (JobWorkerThread*) workThreadObject; // cast void pointer into workerthread object. It gives you the size, the offest, memeber functions etc. A void pointer is ptr to anything. It just a starting point. It could be anything. But casting, makes sure we are dealing with the workerthread object
    thisWorker->Work();
}