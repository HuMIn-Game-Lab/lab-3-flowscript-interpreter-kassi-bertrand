#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>

#include "../lib/jobsystem.h"

#include "parsingjob.h" 

namespace fs = std::filesystem;

struct Diagnostic {
    std::string fileName;
    int lineNumber;
    int column;
    std::string errorType;
    std::string message;
};

void ParsingJob::Execute(){

    // If there are dependencies...
    if (!GetDependencies().empty()) {

        // NOTE: This type of jobs expects only one dependent. The rest is IGNORED.
        int compileJobID = GetDependencies().at(0);
        // Retreive the JSON output of the dependent (i.e. compile job) from the job history located in the job system
        json compileJobOutput = JobSystem::CreateOrGet()->GetJsonJobOutputByID(compileJobID);
        m_content = compileJobOutput["content"];
    } else {
        std::cout << "ERROR: No dependencies: Nothing to parse" << std::endl;
        return;
    }
    
    std::string line;
    std::istringstream error_stream(m_content);
    std::vector< std::vector< std::string > > diagnosticData;

    // Parse warnings and errors, if any...
    while (std::getline(error_stream, line)) {
        
        if (line.find("error") != std::string::npos) {
            // Split line - separator is ":"
            std::vector< std::string > tokens = ParsingJob::splitLine(line, ':');
            diagnosticData.push_back(tokens);
        }

        if (line.find("note") != std::string::npos) {
            // Split line - separator is ":"
            std::vector< std::string > tokens = ParsingJob::splitLine(line, ':');
            diagnosticData.push_back(tokens);
        }

        if (line.find("warning:") != std::string::npos){
            // Split line - separator is ":"
            std::vector< std::string > tokens = ParsingJob::splitLine(line, ':');
            diagnosticData.push_back(tokens);
        }
    }

    // Organize the data into a map with file names as keys and a vector of diagnostics as values
    std::map<std::string, std::vector<Diagnostic>> fileDiagnostics;

    for (const auto& diagnosticInfo : diagnosticData) {
        if (diagnosticInfo.size() >= 5){
            Diagnostic diagnostic;
            diagnostic.fileName = diagnosticInfo[0];
            diagnostic.lineNumber = std::stoi(diagnosticInfo[1]);
            diagnostic.column = std::stoi(diagnosticInfo[2]);
            diagnostic.errorType = diagnosticInfo[3];
            diagnostic.message = diagnosticInfo[4];

            // If there are more elements, append them to diagnostic.message
            for (size_t i = 5; i < diagnosticInfo.size(); ++i) {
                diagnostic.message += " " + diagnosticInfo[i];
            }

            fileDiagnostics[diagnostic.fileName].push_back(diagnostic); // Add diagnostic to the list of diagnostics associated with the file
        }
        //  TODO-1: Make it more robust in case the errors are not formatted as you expect
        //  TODO-1(continued): for instance, there are types of errors where the line and column numbers are not mentioned
        //  TODO-2: Make it so ALL types of diagnostics from clang++ can be parsed.
        //  - Ignored
        //  - Note ✅
        //  - Remark
        //  - Warning ✅
        //  - Error ✅
        //  - Fatal
    }

    // Populate "m_parsedContent" with the file diagnostics
    
    for(const auto& entry: fileDiagnostics){

        const std::string& fileName = entry.first;
        const std::vector<Diagnostic>& diagnostics = entry.second; // All the diagnostics associated with this filename

        json fileData; // JSON array for each file
        for (const Diagnostic& diagnostic : diagnostics){
            json diagnosticJson;
            diagnosticJson["lineNumber"] = diagnostic.lineNumber;
            diagnosticJson["column"] = diagnostic.column;
            diagnosticJson["errorType"] = ParsingJob::trim(diagnostic.errorType);
            diagnosticJson["message"] = ParsingJob::trim(diagnostic.message);

            fileData.push_back(diagnosticJson);
        }

        m_parsedContent[fileName] = fileData; // Associate file name with its JSON array of diagnostics
    }

    // Set Parsing Job Output
    json parsingOutputJson;
    parsingOutputJson["jobChannels"] = 1073741824; // 0x40000000
    parsingOutputJson["jobType"] = 3;
    parsingOutputJson["jsonContent"] = m_parsedContent;
    setOutputJson(parsingOutputJson);
}

void ParsingJob::JobCompleteCallback(){
    // Write the output of this job, in the "Data" folder. NOTE: It is guaranteed to exist. But we never know
    fs::path dataFolderPath = "./Data/";

    if (!fs::exists(dataFolderPath)) {
        fs::create_directories(dataFolderPath);
        std::cout << "Subdirectory created: " << dataFolderPath << std::endl;
    }

    std::string fileName = "ParsingJob-" + std::to_string(GetUniqueID()) + "-output.txt";
    fs::path filePath = dataFolderPath / fileName;

    std::ofstream file(filePath);
    if (file.is_open()) {
        file << m_parsedContent.dump(4);
        file.close();
        // std::cout << "File created: " << filePath << std::endl;
    } else {
        std::cerr << "Failed to create the file: " << filePath << std::endl;
    }
}

std::vector<std::string> ParsingJob::splitLine(const std::string& line, char delimiter) {
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

// Function to remove leading and trailing whitespace from a string
std::string ParsingJob::trim(const std::string& input) {
    // Find the first non-whitespace character from the beginning
    auto start = input.begin();
    while (start != input.end() && std::isspace(*start)) {
        ++start;
    }

    // Find the first non-whitespace character from the end
    auto end = input.end();
    while (end != start && std::isspace(*(end - 1))) {
        --end;
    }

    // Create a new string without leading/trailing whitespace
    return std::string(start, end);
}

void ParsingJob::setOutputJson(const json& json) {
    m_outputJson = json;
}

json ParsingJob::GetOutputJson() const {
    return m_outputJson;
}