#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

#include "../lib/jobsystem.h"

#include "compilejob.h"
#include "parsingjob.h"

namespace fs = std::filesystem;

// Define the constructor
CompileJob::CompileJob(const char* jsonData)
    : Job(jsonData)
{
    json jsonObject = json::parse(jsonData);
    std::string makefile = jsonObject.value("makefile", "");
    bool isFilePath = jsonObject.value("isFilePath", true);

    if(isFilePath){
        std::ifstream fileStream(makefile);
        if(fileStream.is_open()){
            std::string line;
            while (std::getline(fileStream, line)){
                m_makefileContent += line + '\n';
            }
            fileStream.close();
        }
        else{
            std::cerr << "Unable to open file: " << makefile << std::endl;
        }
    }
    else{
        m_makefileContent = makefile;
    }
}

void CompileJob::Execute(){
    std::array<char, 128> buffer;

    // NOTE: I was using the same file name "temp_file" for all the thread. Result? Race condition
    // I was getting inconsistent results. Either an error, or the same output for both job, even though
    // they were handling different files. Now, temporary files will have the job id appended to it to avoid
    // collisions.
    std::string tempFileName = "temp_makefile_"+ std::to_string(GetUniqueID());
    std::ofstream tempFile(tempFileName);
    if (!tempFile.is_open()) {
        std::cerr << "Error: Unable to create a temporary Makefile." << std::endl;
        return;
    }
    tempFile << m_makefileContent;
    tempFile.close();
    std::string command = "make -f " + tempFileName;
    
    // Redirect cerr (2) to cout (&1)
    command.append(" 2>&1");

    // Open pipe and run the command
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cout << "popen Failed: Failed to open file" << std::endl;
        return;
    }

    // Read until the end of the process
    while (fgets(buffer.data(), 128, pipe) != NULL) {
        this->m_compilationOutput.append(buffer.data());
    }

    // Close the pipe and get the return code
    this->returnCode = pclose(pipe);

    // Clean up the temporary file
    std::remove( tempFileName.c_str() );

    // Set the output of the job
    json compilationOutputJson;
    compilationOutputJson["jobChannels"] = 536870912; // 0x20000000
    compilationOutputJson["jobType"] = 2;
    compilationOutputJson["content"] = m_compilationOutput;
    setOutputJson(compilationOutputJson);
}

void CompileJob::JobCompleteCallback(){
    // Write the output of this job, in the "Data" folder. NOTE: It is guaranteed to exist. But we never know lol
    fs::path dataFolderPath = "./Data/";

    if (!fs::exists(dataFolderPath)) {
        fs::create_directories(dataFolderPath);
        std::cout << "Subdirectory created: " << dataFolderPath << std::endl;
    }

    std::string fileName = "CompileJob-" + std::to_string(GetUniqueID()) + "-output.txt";
    fs::path filePath = dataFolderPath / fileName;

    std::ofstream file(filePath);
    if (file.is_open()) {
        file << m_compilationOutput;
        file.close();
        // std::cout << "File created: " << filePath << std::endl;
    } else {
        std::cerr << "Failed to create the file: " << filePath << std::endl;
    }
}

void CompileJob::setOutputJson(const json& json){
    m_outputJson = json;
}

json CompileJob::GetOutputJson() const{
    return m_outputJson;
}

extern "C" {
    void* CreateCompileJob(const char* jsonData) {
        return new CompileJob(jsonData);
    }
}