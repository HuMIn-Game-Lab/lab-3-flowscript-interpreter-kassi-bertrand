#include "../lib/job.h"
#include "../lib/json.hpp"

using json = nlohmann::json;

class CompileJob: public Job{
public:

    // NOTE:    Compile Job accepts either path to make file or its content
    CompileJob(const char* jsonData = nullptr);
    ~CompileJob(){};

    // Polymorphic methods Inherited from the "Job"
    void Execute();
    void JobCompleteCallback();
    void setOutputJson(const json& outputJson);
    json GetOutputJson() const;

private:
    std::string     m_makefileContent;

    int             returnCode; 
    std::string     m_compilationOutput;

    json m_outputJson;
};