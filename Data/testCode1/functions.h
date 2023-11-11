#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>

enum class WarningType {
    INFO,
    WARNING,
    ERROR
};

void generateWarning(WarningType type, const std::string& message);
void generateWarning(int unused); // Function prototype with unused parameter

#endif bad// FUNCTIONS_H
