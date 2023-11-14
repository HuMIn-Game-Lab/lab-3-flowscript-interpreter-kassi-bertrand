#include "conditionaljob.h"
#include "../lib/jobsystem.h"

#include<vector>
#include <filesystem>
#include <fstream>


namespace fs = std::filesystem;

//json data shape
//  {
//     "type": "CONDITIONAL_JOB".encode('utf-8'),
//     "dependencies": [],
//     "input": {
//         "logicalOperation": "LOGICAL_OP",
//         "if_true_job_type": "JOB_TYPE",
//         "else_type_job_type": "JOB_TYPE",
//         "if_true_input": "JSON_INPUT",
//         "else_input": "JSON_INPUT",
//     }
// }
LogicalConditionalJob::LogicalConditionalJob(const char* jsonData): Job(jsonData){
    json jsonObject = json::parse(jsonData);
    
    m_logicalOperation = ParseLogicOperation(jsonObject["logicalOperation"]);
    m_if_true_job_type = jsonObject["if_true_job_type"];
    m_else_type_job_type = jsonObject["else_type_job_type"];
    m_if_true_json_input = jsonObject["if_true_input"];
    m_else_json_input = jsonObject["else_input"];

    // m_targetCount = jsonObject.value("targetCount", 1);
}

void LogicalConditionalJob::Execute(){
    if(GetDependencies().empty()){
        std::cout << "Conditional job without dependencies. Nothing to work with" << std::endl;
        return;
    }
    
    if (EvaluateLogicalCondition()){
        // Conditional Met queue job
        Job* job = JobSystem::CreateOrGet()->CreateJob(m_if_true_job_type, json::parse(m_if_true_json_input.c_str()));
        JobSystem::CreateOrGet()->QueueJob(job);
    }
    else{
        Job* job = JobSystem::CreateOrGet()->CreateJob(m_else_type_job_type, json::parse(m_else_json_input.c_str()));
        JobSystem::CreateOrGet()->QueueJob(job);
    }
}

bool LogicalConditionalJob::EvaluateLogicalCondition() const{
    // Get dependencies; statuses
    std::vector<int> dependencies = GetDependencies();
    std::vector<std::string> statuses;

    for(int jobId: dependencies){
        // Get the status of each dependency job
        json outputJson = JobSystem::CreateOrGet()->GetJsonJobOutputByID(jobId);
        std::string status = outputJson["status"];
        statuses.push_back(status);
    }

    // Perform the specified logical operation
    switch (m_logicalOperation) {
        case LogicalOperation::ALL_SUCCESS:
            return std::all_of(statuses.begin(), statuses.end(), [](const std::string& status) { return status == "success"; });
        case LogicalOperation::ANY_SUCCESS:
            return std::any_of(statuses.begin(), statuses.end(), [](const std::string& status) { return status == "success"; });
        case LogicalOperation::ALL_FAILURE:
            return std::all_of(statuses.begin(), statuses.end(), [](const std::string& status) { return status == "failure"; });
        case LogicalOperation::ANY_FAILURE:
            return std::any_of(statuses.begin(), statuses.end(), [](const std::string& status) { return status == "failure"; });
        case LogicalOperation::N_SUCCESS:
            return std::count(statuses.begin(), statuses.end(), "success") >= m_targetCount;
        case LogicalOperation::N_FAILURE:
            return std::count(statuses.begin(), statuses.end(), "failure") >= m_targetCount;
    }

    return false; // Invalid operation, consider condition not met

}

LogicalConditionalJob::LogicalOperation LogicalConditionalJob::ParseLogicOperation(const std::string& operation) const {
    if (operation == "ALL_SUCCESS") return LogicalOperation::ALL_SUCCESS;
    if (operation == "ANY_SUCCESS") return LogicalOperation::ANY_SUCCESS;
    if (operation == "ALL_FAILURE") return LogicalOperation::ALL_FAILURE;
    if (operation == "ANY_FAILURE") return LogicalOperation::ANY_FAILURE;
    if (operation == "N_SUCCESS") return LogicalOperation::N_SUCCESS;
    if (operation == "N_FAILURE") return LogicalOperation::N_FAILURE;
    return LogicalOperation::ALL_SUCCESS;
}

void LogicalConditionalJob::JobCompleteCallback(){
    // Write the output of this job, in the "Data" folder. NOTE: It is guaranteed to exist. But we never know
    fs::path dataFolderPath = "./Data/";

    if (!fs::exists(dataFolderPath)) {
        fs::create_directories(dataFolderPath);
        std::cout << "Subdirectory created: " << dataFolderPath << std::endl;
    }

    std::string fileName = "ConditionalJob-" + std::to_string(GetUniqueID()) + "-output.txt";
    fs::path filePath = dataFolderPath / fileName;

    std::ofstream file(filePath);
    if (file.is_open()) {
        file << "Conditional job ran successfully";
        file.close();
        // std::cout << "File created: " << filePath << std::endl;
    } else {
        std::cerr << "Failed to create the file: " << filePath << std::endl;
    }

}

void LogicalConditionalJob::setOutputJson(const json& json){
    m_outputJson = json;
}

json LogicalConditionalJob::GetOutputJson() const {
    return m_outputJson;
}
