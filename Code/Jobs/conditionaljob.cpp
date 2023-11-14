#include "conditionaljob.h"
#include "../lib/jobsystem.h"

#include<vector>

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
    m_logicalOperation = ParseLogicOperation(jsonObject.value("logicalOperation", "ALL_SUCCESS"));
    m_targetCount = jsonObject.value("targetCount", 1);
}

void LogicalConditionalJob::Execute(){
    if(GetDependencies().empty()){
        std::cout << "Conditional job without dependencies. Nothing to work with" << std::endl;
        return;
    }
    
    if (EvaluateLogicalCondition()){
        // Conditional Met queue job
    }
    else{
        // Conditional not met queue another job for execution
    }
}

bool LogicalConditionalJob::EvaluateLogicalCondition() const{
    // Get dependencies; statuses
    std::vector<int> dependencies = GetDependencies();
    std::vector<std::string> statuses;

    for(int jobId: dependencies){
        // Get the status of each dependency job
        json outputJson = JobSystem::CreateOrGet()->GetJsonJobOutputByID(jobId);
        std::string status = outputJson.value("status", "failure");
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

}

void LogicalConditionalJob::setOutputJson(const json& json){
    m_outputJson = json;
}

json LogicalConditionalJob::GetOutputJson() const {
    return m_outputJson;
}
