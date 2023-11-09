#pragma once // Make sure files do not get included
#include <mutex>
#include <map>
#include <deque>
#include <vector>
#include <thread>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

class Job
{
    friend class JobSystem;
    friend class JobWorkerThread;

public:
    Job(const char* jsonData = nullptr){

        json jsonObject = json::parse(jsonData);

        m_jobChannels = jsonObject.value("jobChannels", 0xFFFFFFFF);
        m_jobType = jsonObject.value("jobType", -1);
        
        static int s_nextJobID = 0;
        m_jobID = s_nextJobID++;
    }

    virtual ~Job() {}
    virtual void Execute() = 0;
    virtual void JobCompleteCallback() = 0;
    int GetUniqueID() const { return m_jobID; } // const functions... cannot modify stuff in the class.
    virtual void setOutputJson(const json& outputJson) = 0;
    virtual json GetOutputJson() const = 0;

    void AddDependency(int jobId){
        m_dependencies.push_back(jobId);
    }

    const std::vector<int>& GetDependencies() const{
        return m_dependencies;
    }

private:
    int m_jobID = -1;
    int m_jobType = -1;
    unsigned long m_jobChannels = 0xFFFFFFFF;
    std::vector<int> m_dependencies; // The jobs who needs to complete BEFORE this one runs.
};