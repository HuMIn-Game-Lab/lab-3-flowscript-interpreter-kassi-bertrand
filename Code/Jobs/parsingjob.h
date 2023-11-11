#include "../lib/job.h"
#include "../lib/json.hpp"
#include "../lib/jobsystem.h"

using json = nlohmann::json;

class ParsingJob: public Job{
public:    
    ParsingJob(const char* jsonData = nullptr): Job(jsonData){
        json jsonObject = json::parse(jsonData);
        m_content = jsonObject.value("content", "");
    }
    ~ParsingJob(){};

    void Execute();
    void JobCompleteCallback();
    void setOutputJson(const json& outputJson);
    json GetOutputJson() const;

private:
    std::string     m_content;
    json            m_parsedContent;

    static std::vector<std::string> splitLine(const std::string& line, char delimiter);
    static std::string trim(const std::string& input);

    json m_outputJson;
};