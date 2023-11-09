#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include "../lib/jobsystem.h"
#include "../lib/json.hpp"

#include "compilejob.h"
#include "parsingjob.h"
#include "jsonjob.h"

int main(int argc, char const *argv[])
{
    std::system("clear");

    std::cout << "Creating Job System" << std::endl;
    JobSystemHandle js = GetJobSystemInstance();

    std::vector<JobHandle> jobs;

    std::cout << "Creating Worker Threads" << std::endl;
    CreateWorkerThreads(js);
    
    std::cout << "Create compile jobs" << std::endl;
    std::string compile_1 = (R"(
        {
            "jobChannels": 268435456,
            "jobType": 1,
            "makefile": "./Data/testCode/Makefile",
            "isFilePath": true
        }
    )");

    std::string parsing_1 = (R"(
        {
            "jobChannels": 536870912,
            "jobType": 2,
            "content": ""
        }
    )");

    std::string json_1 = (R"(
        {
            "jobChannels": 1073741824,
            "jobType": 3,
            "jsonContent": ""
        }
    )");

    // Register new job types using exposed library functions
    RegisterJobType(js, "COMPILE_JOB",  [](const char* jsonData) -> void* { return new CompileJob(jsonData); });
    RegisterJobType(js, "PARSING_JOB",  [](const char* jsonData) -> void* { return new ParsingJob(jsonData);});
    RegisterJobType(js, "JSON_JOB",     [](const char* jsonData) -> void* { return new JsonJob(jsonData);});

    // Insantiate each type using function from exposed library
    JobHandle compile_jb1    = CreateJob(js, "COMPILE_JOB", compile_1.c_str());
    JobHandle parsing_jb1    = CreateJob(js, "PARSING_JOB", parsing_1.c_str());
    JobHandle json_jb1       = CreateJob(js, "JSON_JOB", json_1.c_str());
    
    // Specify dependencies using function from exposed libary
    AddDependency(parsing_jb1, compile_jb1); // Parsing job will run after compile job is complete
    AddDependency(json_jb1, parsing_jb1); // Json job will run after parsing job completes

    // Queue the jobs for executation
    jobs.push_back(compile_jb1);
    jobs.push_back(parsing_jb1);
    jobs.push_back(json_jb1);

    std::cout << "Queueing jobs" << std::endl;
    std::vector<JobHandle>::iterator it = jobs.begin();

    for(; it!= jobs.end(); ++it){
        QueueJob(js, *it);
    }

    // Event loop
    int running = 1;

    while(running){
        std::string command;
        std::cout << "Enter: \"stop\", \"destroy\", \"finish\", \"status\", \"finishjob\", or \"job_types\", \"history\": \n";
        std::cin >> command;

        if (command == "stop")
        {
            running = 0;
        }
        else if (command == "destroy")
        {
            FinishCompletedJobs(js);
            DestroyJobSystemInstance(js);
            running = 0;
        }
        else if (command == "finish")
        {
            FinishCompletedJobs(js);
        }
        else if (command == "status")
        {
            int jobID = 0;
            std::string str;
            std::cout << "Enter ID of job to get status: ";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear input buffer
            std::getline(std::cin, str);
            std::stringstream sstr(str);
            sstr >> jobID;
            std::cout << "Status for job (# " << jobID << ") is: " << GetJobStatus(js, jobID) << std::endl;
        }
        else if(command == "finishjob"){
            int jobID = 0;
            std::string str;
            std::cout << "Enter ID of job to finish: ";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear input buffer
            std::getline(std::cin, str);
            std::stringstream sstr(str);
            sstr >> jobID;
            FinishJob(js, jobID);
            //std::cout << "After finishing job 0, status is: " << js->GetJobStatus(0) << std::endl;
        }
        else if (command == "job_types"){
            char** jobTypeArray = GetAvailableJobTypes(js);
            std::cout << '\n';
            for(int i = 0; jobTypeArray[i] != nullptr; i++){
                std::cout << "Job type " << i << ": " << jobTypeArray[i] << '\n';
            }
            std::cout << '\n';
            FreeJobTypesArray(jobTypeArray);
        }
        else if (command == "history"){
            GetJobDetails(js);
        }
        else{
            std::cout << "Invalid command" << std::endl;
        }
        
    }
    return 0;
}