#include "jsonjob.h"
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

void JsonJob::Execute(){
    // If there are dependencies...
    if (!GetDependencies().empty()) {
        // NOTE: This type of jobs expects only one dependent. The rest is IGNORED.
        int compileJobID = GetDependencies().at(0);
        // Retreive the JSON output of the dependent (i.e. compile job) from the job history located in the job system
        json compileJobOutput = JobSystem::CreateOrGet()->GetJsonJobOutputByID(compileJobID);
        m_json = compileJobOutput["jsonContent"];
    } else {
        m_json["status"] = "failure";
        setOutputJson(m_json);
        std::cout << "ERROR: No dependencies: Nothing to work with" << std::endl;
        return;
    }

    // Extract file name and line number from the JSON
    for (auto& entry: m_json.items()) {
        auto& fileName = entry.key();
        auto& diagnostics = entry.value();

        for (auto& diagnostic : diagnostics) {
            int lineNumber = diagnostic["lineNumber"];
            
            // Read lines before and after the specified line number
            auto contextLines = readContextLines(fileName, lineNumber);

            // Add the context lines to the JSON object
            diagnostic["contextBefore"] = contextLines.first;
            diagnostic["contextAfter"] = contextLines.second;
        }
    }

    // Set JSON job output
    m_json["status"] = "success";
    setOutputJson(m_json);
}

void JsonJob::JobCompleteCallback(){
    // Write the output of this job, in the "Data" folder. NOTE: It is guaranteed to exist. But we never know
    fs::path dataFolderPath = "./Data/";

    if (!fs::exists(dataFolderPath)) {
        fs::create_directories(dataFolderPath);
        std::cout << "Subdirectory created: " << dataFolderPath << std::endl;
    }

    std::string fileName = "JsonJob-" + std::to_string(GetUniqueID()) + "-output.txt";
    fs::path filePath = dataFolderPath / fileName;

    std::ofstream file(filePath);
    if (file.is_open()) {
        file << m_json.dump(4);
        file.close();
        // std::cout << "File created: " << filePath << std::endl;
    } else {
        std::cerr << "Failed to create the file: " << filePath << std::endl;
    }
}

std::pair<std::string, std::string> JsonJob::readContextLines(const std::string& fileName, int lineNumber){
    
    std::ifstream file(fileName);
    if (!file.is_open()) {
        return {"", ""}; // Return empty strings if the file cannot be opened
    }

    std::string line;
    std::string contextBefore = "";
    std::string contextAfter = "";
    int currentLineNumber = 0;

    while (std::getline(file, line)) {
       currentLineNumber++;
        if (currentLineNumber >= lineNumber - 2 && currentLineNumber < lineNumber) {
            contextBefore += line + "\n";
        } else if (currentLineNumber > lineNumber && currentLineNumber <= lineNumber + 2) {
            contextAfter += line + "\n";
        }
    }

    return {contextBefore, contextAfter};
}

void JsonJob::setOutputJson(const json& json){
    m_outputJson = json;
}

json JsonJob::GetOutputJson() const {
    return m_outputJson;
}