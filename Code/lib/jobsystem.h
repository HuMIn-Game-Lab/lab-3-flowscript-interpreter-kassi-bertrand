#pragma once
#include <iostream>
#include <mutex>
#include <map>
#include <deque>
#include <vector>
#include <thread>
#include <functional>
#include "json.hpp"

using json = nlohmann::json;

constexpr int JOB_TYPE_ANY = -1;

class JobWorkerThread; // Forward declaration, tell jobsystem that it should be aware of but is actually implemented somewhere else. If the compiler do not find it, we get an error.

enum JobStatus
{
    JOB_STATUS_NEVER_SEEN,
    JOB_STATUS_QUEUED,
    JOB_STATUS_RUNNING,
    JOB_STATUS_COMPLETED,
    JOB_STATUS_RETIRED,
    NUM_JOB_STATUSES
};

struct JobHistoryEntry
{
    JobHistoryEntry(int jobID, int jobType, JobStatus jobStatus) : m_jobID(jobID), m_jobType(jobType), m_jobStatus(jobStatus) {}

    int m_jobID = -1;
    int m_jobType = -1;
    int m_jobStatus = JOB_STATUS_NEVER_SEEN;
    json m_jobOutput; // Will store the output of jobs
};

class Job; // Another forward declaration

class JobSystem
{
    friend JobWorkerThread;

public:
    ~JobSystem();

    static JobSystem* CreateOrGet();
    static void Destroy();
    int totalJobs = 0;
    int jobqueued = 0;
    int jobrunning = 0;
    int jobcompleted = 0;
    int jobretired = 0;

    void FinishCompletedJobs();
    void FinishJob(int jobID);

    void CreateWorkerThread(const char *uniqueName, unsigned long workerJobChannels = 0xFFFFFFFF);
    void DestroyWorkerThread(const char *uniqueName);
    static const char* generateRandomThreadWorkerName(int length = 3); // I don't want to have to name them everytime I create a worker thread
    void QueueJob(Job *job); // Sets the status of the job to "QUEUED" and adds it to the "m_jobsQueued" vector.
    json GetJsonJobOutputByID(int jobID) const;

    // Status Queries
    JobStatus GetJobStatus(int jobID) const;
    bool isJobComplete(int jobID) const; // OLD NAME: isComplete

    void GetJobDetails() const;

    void RegisterJobType(const std::string& jobTypeIdentifier, std::function<Job* (const char*)> jobFactoryFunction){
        auto it = m_jobTypeFactories.find(jobTypeIdentifier);
        if (it == m_jobTypeFactories.end()){
            // The identifier does not already exist, registration can take place
            m_jobTypeFactories[jobTypeIdentifier] = jobFactoryFunction;
        } else {
            std::cout << "Error: Job type with identifier '" << jobTypeIdentifier << "' already registered." << std::endl;
        }
    }

    Job* CreateJob(const std::string& jobTypeIdentifier, const json& jsonData); // Returns an instance of a job based on type identifier. This function implements the FACTORY pattern.

    std::vector<std::string> GetRegisteredJobTypes() const; // Returns a list of registered job types in the job system

private:
    JobSystem();
    
    Job* ClaimAJob(unsigned long workerJobFlags); // go through queued job, and find a job comp with a thread. And move the job queued to running queue
    void OnJobCompleted(Job *jobJustExecuted); // when a thread completes the job, will mve from running queue to completed queue

    static JobSystem *s_jobSystem;

    std::vector<JobWorkerThread *>      m_workerThreads;
    mutable std::mutex                  m_workerThreadsMutex;
    std::deque< Job* >                  m_jobsQueued;
    std::deque< Job* >                  m_jobsRunning;
    std::deque< Job* >                  m_jobsCompleted;
    mutable std::mutex                  m_jobsQueuedMutex;
    mutable std::mutex                  m_jobsRunningMutex;
    mutable std::mutex                  m_jobsCompletedMutex;

    std::vector< JobHistoryEntry >      m_jobHistory;
    mutable int                         m_jobHistoryLowestActiveIndex = 0; // The index of the oldest thread that is still running. Because JobID will only keep increasing.
    mutable std::mutex                  m_jobHistoryMutex;

    std::map<std::string, std::function<Job* (const char*)> > m_jobTypeFactories; // associate a string, with a function template that can store callable (function in our case) that takes a constant ref to a JSON and returns a pointer to a Job instance.
};

// Define JobSystemHandle and JobHandle as void pointers
typedef void* JobSystemHandle;
typedef void* JobHandle;

extern "C"{
    // Start - Destroy job system
    JobSystemHandle GetJobSystemInstance();
    void DestroyJobSystemInstance(JobSystemHandle jobSystem);

    // Create worker threads
    void CreateWorkerThreads(JobSystemHandle jobSystem);

    // Create, Complete, queue, query status jobs, add dependency
    JobHandle CreateJob(JobSystemHandle jobsystem, const char* jobTypeIdentifier, const char* jsonData);
    void FinishJob(JobSystemHandle jobsystem, int jobID);
    void FinishCompletedJobs(JobSystemHandle jobsystem);
    void QueueJob(JobSystemHandle jobsystem, JobHandle jobHandle);
    int GetJobStatus(JobSystemHandle jobsystem, int jobID);
    void AddDependency(JobHandle dependentHandle, JobHandle dependencyHandle);

    // Register job types
    void RegisterJobType(JobSystemHandle jobsystem, const char* jobIdentifier, void* (*jobFactoryFunction)(const char*));

    // Get list of available job types
    char** GetAvailableJobTypes(JobSystemHandle jobsystem);
    void FreeJobTypesArray(char** jobTypesArray);

    // Job details
    void GetJobDetails(JobSystemHandle jobsystem);


}