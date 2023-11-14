#include <cstring>
#include <cstdlib>
#include <ctime>
#include <iomanip>

#include "jobsystem.h"
#include "jobworkerthread.h"

#include "../Jobs/compilejob.h"
#include "../Jobs/parsingjob.h"
#include "../Jobs/jsonjob.h"
#include "../Jobs/conditionaljob.h"

// static variable are initialized in the cpp
JobSystem* JobSystem::s_jobSystem = nullptr;

typedef void (*JobCallBack)(Job* completedJob); // JobCallBack is a type describing a ptr func that point to a function accepting a job, and that returns a void

JobSystem::JobSystem(){
    m_jobHistory.reserve( 256 * 1024 ); // Reserves a big chunk of memory. Why? Talk to Dr. Clorey. But it gives us much higher runtime performance.
}

JobSystem::~JobSystem(){
    m_workerThreadsMutex.lock();
    int numWorkerThreads = (int)m_workerThreads.size();

    // First, tell each work thread worker to stop picking up jobs
    for(int i = 0; i < numWorkerThreads; i++){
        m_workerThreads[i]->ShutDown();
    }

    while( !m_workerThreads.empty() ){ // As long as the vector is not empty
        delete m_workerThreads.back(); // Deallocate the last worker thread in the vector
        m_workerThreads.pop_back(); // Decrease the vector. If the above step was not, performed... memory leak.
    }
    m_workerThreadsMutex.unlock();
}

JobSystem* JobSystem::CreateOrGet(){
    if( !s_jobSystem ){
        s_jobSystem = new JobSystem();
    }

    return s_jobSystem;
}

void JobSystem::Destroy(){
    if(s_jobSystem){
        delete s_jobSystem;
        s_jobSystem = nullptr;
    }
}

void JobSystem::CreateWorkerThread(const char* uniqueName, unsigned long workerJobChannels){
    JobWorkerThread* newWorker = new JobWorkerThread( uniqueName, workerJobChannels, this);

    m_workerThreadsMutex.lock();
    m_workerThreads.push_back(newWorker);
    m_workerThreadsMutex.unlock();

    m_workerThreads.back()->StartUp();
}

void JobSystem::DestroyWorkerThread(const char* uniqueName){
    m_workerThreadsMutex.lock();
    JobWorkerThread* doomedWorker = nullptr;
    std::vector<JobWorkerThread*>::iterator it = m_workerThreads.begin();

    for(; it != m_workerThreads.end(); ++it){
        if (strcmp( (*it)->m_uniqueName, uniqueName) == 0){
            doomedWorker = *it;
            m_workerThreads.erase(it);
            break;
        }
    }

    m_workerThreadsMutex.unlock();

    if(doomedWorker){
        doomedWorker->ShutDown();
        delete doomedWorker;
    }
}

void JobSystem::QueueJob(Job* job){
    m_jobsQueuedMutex.lock();

    m_jobHistoryMutex.lock();
    m_jobHistory.emplace_back(JobHistoryEntry(job->GetUniqueID(), job->m_jobType, JOB_STATUS_QUEUED));
    //increase job queued
    jobqueued++;
    
    m_jobHistoryMutex.unlock();

    m_jobsQueued.push_back(job);
    m_jobsQueuedMutex.unlock();
}

JobStatus JobSystem::GetJobStatus(int jobID) const{
    m_jobHistoryMutex.lock();

    JobStatus jobStatus = JOB_STATUS_NEVER_SEEN;
    if (jobID < (int) m_jobHistory.size()){
        jobStatus = (JobStatus)(m_jobHistory[jobID].m_jobStatus);
    }

    m_jobHistoryMutex.unlock();
    return jobStatus;
}

bool JobSystem::isJobComplete(int jobID) const{
    return (GetJobStatus(jobID)) == (JOB_STATUS_COMPLETED);
}

// NOTE:    The content of "m_jobsCompleted" is copied into the
//          "jobsCompleted" vector. We iterate over the pointers
//          in this copy, mark them as "retired," and deallocate
//          them. However, this operation does not affect the
//          original "m_jobsCompleted" vector, which may still
//          contain dangling pointers.

// FIX:     Directly iterate over "m_jobsCompleted", mark each job
//          as "retired", deallocate, then finally erase the job from
//          vector. Or we can iterate, mark each job as "retired"...
//          then after the iteration... empty the vector.
void JobSystem::FinishCompletedJobs(){
    std::deque<Job*> jobsCompleted;

    m_jobsCompletedMutex.lock();
    jobsCompleted.swap(m_jobsCompleted); // Copies the content of the "Job System jobCompleted" queue into a local copy, where the thread can perform all of its manipulations, without monopolizing the job system queue which is a shared resource
    m_jobsCompletedMutex.unlock();

    for(Job* job: jobsCompleted){
        job->JobCompleteCallback();
        m_jobHistoryMutex.lock();
        m_jobHistory[job->m_jobID].m_jobStatus = JOB_STATUS_RETIRED; 
        // increase "jobretired" and decrease "jobcompleted"
        jobretired++;
        jobcompleted--;
        
        m_jobHistoryMutex.unlock();
        delete job;
    }

}

void JobSystem::FinishJob(int jobID){
    // NOTE: The code after the while loop was within the body of the loop
    // NOTE: This implementation differs from the class.

    // NOTE:    The check inside the while loop ensures that trying to "finish" a job
    //          that does not exist or that has already been "finished", does not cause the program
    //          to hang forever in the loop.

    // As long as the specified job is NOT with a "COMPLETE" status          
    while(!isJobComplete(jobID)){
        JobStatus jobStatus = GetJobStatus(jobID);
        if((jobStatus == JOB_STATUS_NEVER_SEEN) || (jobStatus == JOB_STATUS_RETIRED)){
            std::cout << "Error: Waiting for job (# " << jobID << ") - no such job in JobSystem" << std::endl;
            return; 
        }
    }

    m_jobsCompletedMutex.lock();
    Job* thisCompletedJob = nullptr;
    for(auto jcIter = m_jobsCompleted.begin(); jcIter != m_jobsCompleted.end(); ++jcIter){
        Job* someCompletedJob = *jcIter;
        if(someCompletedJob->m_jobID == jobID){
            thisCompletedJob = someCompletedJob;
            m_jobsCompleted.erase(jcIter);
            break;
        }
    }
    m_jobsCompletedMutex.unlock();

    if(thisCompletedJob == nullptr){
        std::cout << "ERROR: Job # " << jobID << " was status completed but not found" << std::endl;
    }

    thisCompletedJob->JobCompleteCallback();

    m_jobHistoryMutex.lock();
    m_jobHistory[thisCompletedJob->m_jobID].m_jobStatus = JOB_STATUS_RETIRED;
    // increase "jobretired", decrease "jobcompleted"
    jobretired++;
    jobcompleted--;

    m_jobHistoryMutex.unlock();

    delete thisCompletedJob;
}

void JobSystem::OnJobCompleted(Job* jobJustExecuted){
    totalJobs++;
    m_jobsCompletedMutex.lock();
    m_jobsRunningMutex.lock();

    std::deque<Job*>::iterator runningJobItr = m_jobsRunning.begin();
    for(; runningJobItr != m_jobsRunning.end(); ++runningJobItr){
        if(jobJustExecuted == *runningJobItr){
            m_jobHistoryMutex.lock();
            m_jobsRunning.erase(runningJobItr);
            m_jobsCompleted.push_back(jobJustExecuted);
            m_jobHistory[jobJustExecuted->m_jobID].m_jobStatus = JOB_STATUS_COMPLETED;
            // Save the ouptut of the job in the job history as well
            m_jobHistory[jobJustExecuted->m_jobID].m_jobOutput = jobJustExecuted->GetOutputJson();
            //decrease "jobrunning" and increase "jobcompleted"
            jobrunning--;
            jobcompleted++;
            m_jobHistoryMutex.unlock();
            break;
        }
    }
    m_jobsRunningMutex.unlock();
    m_jobsCompletedMutex.unlock();
}

Job* JobSystem::ClaimAJob(unsigned long workerJobChannels){
    m_jobsQueuedMutex.lock();
    m_jobsRunningMutex.lock();

    Job* claimedJob = nullptr;
    std::deque<Job*>::iterator queuedJobIter = m_jobsQueued.begin();
    for(; queuedJobIter != m_jobsQueued.end(); ++queuedJobIter){
        Job* queuedJob = *queuedJobIter;

        if( (queuedJob->m_jobChannels & workerJobChannels) != 0){ // There was a match
            bool dependenciesCompleted = true;
            
            // Make sure the dependencies of the job TO BE claimed are ALL in "COMPLETE STATUS"
            for(int dependencyId: queuedJob->GetDependencies()){
                JobStatus dependencyStatus = GetJobStatus(dependencyId);
                if (dependencyStatus != JOB_STATUS_COMPLETED) {
                    dependenciesCompleted = false;
                    break;
                }
            }

            if (dependenciesCompleted) {
                claimedJob = queuedJob;

                m_jobHistoryMutex.lock();
                m_jobsQueued.erase(queuedJobIter);
                m_jobsRunning.push_back(claimedJob);
                m_jobHistory[claimedJob->m_jobID].m_jobStatus = JOB_STATUS_RUNNING;
                // increase "jobrunning" decrease "jobqueued"
                jobrunning++;
                jobqueued--;

                // FORGOT TO UNLOCK... SO DEADLOCK HAPPENED HERE
                m_jobHistoryMutex.unlock();
                break;
            }
        }
    }

    m_jobsRunningMutex.unlock();
    m_jobsQueuedMutex.unlock();

    return claimedJob;
}

const char* JobSystem::generateRandomThreadWorkerName(int length){
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charsetLength = charset.length();
    char* randomString = new char[length + 1];

    for (int i = 0; i < length; ++i) {
        int randomIndex = std::rand() % charsetLength;
        randomString[i] = charset[randomIndex];
    }

    randomString[length] = '\0';

    return randomString;
}

// TODO: Update to also display the number of jobs queued
void JobSystem::GetJobDetails() const{
    std::cout << "JOBS SUMMARY" << std::endl;
    std::cout << "===========\n" << std::endl;
    
    std::cout << "Total jobs: " << totalJobs << std::endl;
    std::cout << "Job completed: " << jobcompleted << std::endl;
    std::cout << "Job running: " << jobrunning << std::endl;
    std::cout << "Job retired: " << jobretired << std::endl;

    std::cout << "\nDETAILED SUMMARY" << std::endl;
    std::cout << "===========\n" << std::endl;

    // Define column widths
    const int idWidth = 5;
    const int statusWidth = 15;
    const int typeWidth = 10;

    // Display the top horizontal line
    std::cout   << "+" << std::string(idWidth + 2, '-') << "+"
                << std::string(statusWidth + 2, '-') << "+"
                << std::string(typeWidth + 2, '-') << "+" << std::endl;

    // Display a table header with vertical lines
    std::cout   << "| " << std::left << std::setw(idWidth) << "ID" << " | "
                << std::left << std::setw(statusWidth) << "Status" << " | "
                << std::left << std::setw(typeWidth) << "Type" << " |" << std::endl;

    // Display the middle horizontal line
    std::cout   << "+" << std::string(idWidth + 2, '-') << "+"
                << std::string(statusWidth + 2, '-') << "+"
                << std::string(typeWidth + 2, '-') << "+" << std::endl;

    // Iterate through the vector and display each struct as a row in the table
    for(const auto& record: m_jobHistory){
        std::cout   << "| " << std::left << std::setw(idWidth) << record.m_jobID << " | "
                    << std::left << std::setw(statusWidth) << record.m_jobStatus << " | "
                    << std::left << std::setw(typeWidth) << record.m_jobType << " |" << std::endl;
        
        // Display a horizontal line between rows
        std::cout   << "+" << std::string(idWidth + 2, '-') << "+"
                    << std::string(statusWidth + 2, '-') << "+"
                    << std::string(typeWidth + 2, '-') << "+" << std::endl;
    }

    std::cout << std::endl;
}

json JobSystem::GetJsonJobOutputByID(int jobID) const{
    m_jobHistoryMutex.lock();
    json jobOutput;

    // Go through the history, find the job, and if COMPLETED OR RETIRED, return its output
    for (const JobHistoryEntry& entry : m_jobHistory){
        if(entry.m_jobID == jobID && (entry.m_jobStatus == JOB_STATUS_COMPLETED || entry.m_jobStatus == JOB_STATUS_RETIRED)){
            jobOutput = entry.m_jobOutput;
            break;
        }
    }    
    
    m_jobHistoryMutex.unlock();

    return jobOutput;
}

Job* JobSystem::CreateJob(const std::string jobTypeIdentifier, const json& jsonData){
    auto it = m_jobTypeFactories.find(jobTypeIdentifier);
    if(it != m_jobTypeFactories.end()){
        auto& factoryFunction = it->second;
        return factoryFunction(jsonData.dump().c_str());
    } else {
        std::cout << "Error: Job type with identifier: '" << jobTypeIdentifier << "' - not registered." << std::endl;
        return nullptr;
    }
}

std::vector<std::string> JobSystem::GetRegisteredJobTypes() const {
    std::vector<std::string> registeredJobTypes;
    for (const auto& pair : m_jobTypeFactories) {
        registeredJobTypes.push_back(pair.first);
    }
    return registeredJobTypes;
}

extern "C"{
    JobSystemHandle GetJobSystemInstance(){
        return reinterpret_cast<JobSystemHandle>(JobSystem::CreateOrGet());
    }

    void DestroyJobSystemInstance(JobSystemHandle jobSystem){
        if (jobSystem){
            delete reinterpret_cast<JobSystem *>(jobSystem);
            jobSystem = nullptr;
        }
    }

    void CreateWorkerThreads(JobSystemHandle jobSystem){
        JobSystem *js = reinterpret_cast<JobSystem*>(jobSystem);

        if (js == nullptr){
            std::cout << "Job system is NOT intitialized" << std::endl;
           return;
        }

        js->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x10000000); // Dedicated worker threads for COMPILE jobs
        js->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x10000000);

        js->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x20000000); // Dedicated worker threads for PARSING jobs
        js->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x20000000);

        js->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x40000000); // Dedicated worker threads for JSON jobs
        js->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x40000000);

        js->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x8000000); // Dedicated worker threads for ANY other jobs
    }

    JobHandle CreateJob(JobSystemHandle jobSystem, const char* jobTypeIdentifier, const char* jsonData){
        std::string id = jobTypeIdentifier;
        Job* job = reinterpret_cast<JobSystem*>(jobSystem)->CreateJob(id, json::parse(jsonData));
        return reinterpret_cast<JobHandle>(job);
    }

    void FinishJob(JobSystemHandle jobSystem, int jobID){
        reinterpret_cast<JobSystem*>(jobSystem)->FinishJob(jobID);
    }


    void FinishCompletedJobs(JobSystemHandle jobSystem){
        reinterpret_cast<JobSystem*>(jobSystem)->FinishCompletedJobs();
    }


    void QueueJob(JobSystemHandle jobsystem, JobHandle jobHandle){
        Job* job = reinterpret_cast<Job*>(jobHandle);
        reinterpret_cast<JobSystem*>(jobsystem)->QueueJob(job);
    }

    int GetJobStatus(JobSystemHandle jobsystem, int jobID){
        return reinterpret_cast<JobSystem*>(jobsystem)->GetJobStatus(jobID);
    }

    int GetJobID(JobSystemHandle jobsystem, JobHandle jobHandle){
        Job* job = reinterpret_cast<Job*>(jobHandle);
        return job->GetUniqueID();
    }

    void AddDependency(JobHandle dependentHandle, JobHandle dependencyHandle){
        // Consider Job A and Job B. 
        // Job B is must wait for Job A to finish before starting.
        // Job B is the "dependent"
        // Job A is the "dependency"
        Job* dependent = reinterpret_cast<Job*>(dependentHandle);
        Job* dependency = reinterpret_cast<Job*>(dependencyHandle);
        
        dependent->AddDependency(dependency->GetUniqueID());
    }

    void RegisterJobType(JobSystemHandle jobsystem, const char* jobIdentifier, void* (*jobFactoryFunction)(const char*)){
        //
        std::function<Job* (const char*)> factoryFunctionWrapper = [=](const char* jsonData){
            // REMEMBER: the "=" copies by value variables in the surrounding scope that are referenced in the lambda
            
            void* jobHandle = jobFactoryFunction(jsonData);

            return reinterpret_cast<Job*>(jobHandle);
        };

        reinterpret_cast<JobSystem*>(jobsystem)->RegisterJobType(jobIdentifier, factoryFunctionWrapper);
    }

    char** GetAvailableJobTypes(JobSystemHandle jobsystem){
        // Cast the job system back to JobSystem*
        JobSystem* js = reinterpret_cast<JobSystem*>(jobsystem);

        // Get the list of registered job types from the job system
        std::vector<std::string> registeredJobTypes = js->GetRegisteredJobTypes();

        // Allocate memory for an array of char pointers
        char** jobTypeArray = new char*[registeredJobTypes.size()];

        // Copy each job type string to the array
        for(size_t i = 0; i < registeredJobTypes.size(); i++){
            // Allocate memory for each type string and copy the content
            jobTypeArray[i] = new char [registeredJobTypes[i].size() + 1];
            strcpy(jobTypeArray[i], registeredJobTypes[i].c_str());
            jobTypeArray[i][registeredJobTypes[i].size()] = '\0';  // Null-terminate the string
        }

        // Null-terminate the last element of the array
        jobTypeArray[registeredJobTypes.size()] = nullptr;

        return jobTypeArray;
    }

    void FreeJobTypesArray(char** jobTypesArray) {
        // Iterate through the array and deallocate memory for each string
        for (int i = 0; jobTypesArray[i] != nullptr; ++i) {
            delete[] jobTypesArray[i];
        }
        
        // Deallocate memory for the array itself
        delete[] jobTypesArray;
    }


    void GetJobDetails(JobSystemHandle jobsystem){
        reinterpret_cast<JobSystem*>(jobsystem)->GetJobDetails();
    }

    void InitJobSystem(){

        // Register jobs

        std::function<Job* (const char *)> compileJobFactory = [](const char *jsonData) -> Job* {
            return new CompileJob(jsonData);
        };

        std::function<Job* (const char *)> parsingJobFactory = [](const char *jsonData) -> Job* {
            return new ParsingJob(jsonData);
        };

        std::function<Job* (const char *)> jsonJobFactory = [](const char *jsonData) -> Job* {
            return new JsonJob(jsonData);
        };

        std::function<Job* (const char*)> conditionalJobFactory = [](const char* jsonData) -> Job* {
            return new LogicalConditionalJob(jsonData);
        };

        JobSystem::CreateOrGet()->RegisterJobType("COMPILE_JOB", compileJobFactory);
        JobSystem::CreateOrGet()->RegisterJobType("PARSING_JOB", parsingJobFactory);
        JobSystem::CreateOrGet()->RegisterJobType("JSON_JOB", jsonJobFactory);
        JobSystem::CreateOrGet()->RegisterJobType("CONDITIONAL_JOB", conditionalJobFactory);

        // Kick off worker threads
        JobSystem::CreateOrGet()->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x10000000); // Dedicated worker threads for COMPILE jobs
        JobSystem::CreateOrGet()->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x10000000);

        JobSystem::CreateOrGet()->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x20000000); // Dedicated worker threads for PARSING jobs
        JobSystem::CreateOrGet()->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x20000000);

        JobSystem::CreateOrGet()->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x40000000); // Dedicated worker threads for JSON jobs
        JobSystem::CreateOrGet()->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x40000000);

        JobSystem::CreateOrGet()->CreateWorkerThread(JobSystem::generateRandomThreadWorkerName(), 0x8000000); // Dedicated worker threads for ANY other jobs
    }
}