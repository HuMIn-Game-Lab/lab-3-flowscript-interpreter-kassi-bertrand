#include "functions.h"
#include <iostream>

void generateWarning(WarningType type, const std::string& message) {
    std::string warningType;
    switch(type) {
        case WarningType::INFO:
            warningType = "Info";
            break;
        case WarningType::WARNING:
            warningType = "Warning"
            break;
        case WarningType::ERROR:
            warningType = "Error";
            break;
    }
    std::cerr << "[" << warningType << "]: " << message << std::endl;
}

void generateWarning(int unused) {
    // Using the unused parameter to generate a warning
    if (unused) {
        // Do something with unused to avoid "unused variable" warning
    }
}
