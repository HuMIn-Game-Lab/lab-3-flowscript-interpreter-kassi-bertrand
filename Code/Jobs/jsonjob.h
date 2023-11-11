#include "../lib/job.h"
#include "../lib/json.hpp"
#include "../lib/jobsystem.h"

using json = nlohmann::json;

class JsonJob: public Job{
public:
    JsonJob(const char* jsonData = nullptr): Job(jsonData){
        json jsonObject = json::parse(jsonData);
        m_json = jsonObject.value("jsonContent", json{});
    }
    ~JsonJob(){};

    void Execute();
    void JobCompleteCallback();
    void setOutputJson(const json& outputJson);
    json GetOutputJson() const;

private:
    json m_json;
    static std::pair<std::string, std::string> readContextLines(const std::string& fileName, int lineNumber);

    json m_outputJson;
};