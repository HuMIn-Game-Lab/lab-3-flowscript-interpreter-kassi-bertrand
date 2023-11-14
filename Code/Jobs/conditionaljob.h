#include "../lib/job.h"
#include "../lib/json.hpp"
#include "../lib/jobsystem.h"

using json = nlohmann::json;

class LogicalConditionalJob: public Job{
    enum class LogicalOperation {
        ALL_SUCCESS,
        ANY_SUCCESS,
        ALL_FAILURE,
        ANY_FAILURE,
        N_SUCCESS,
        N_FAILURE
    };

    public:
    LogicalConditionalJob(const char* jsonData = nullptr);
    ~LogicalConditionalJob(){};

    void Execute();
    void JobCompleteCallback();
    void setOutputJson(const json& outputJson);
    json GetOutputJson() const;

    private:
    LogicalOperation ParseLogicOperation(const std::string& operation) const;
    bool EvaluateLogicalCondition() const;

    LogicalOperation m_logicalOperation;
    int m_targetCount;


    json m_outputJson;
};