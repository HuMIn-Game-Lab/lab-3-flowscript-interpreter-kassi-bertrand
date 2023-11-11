#include "functions.h"

int main() {
    generateWarning(WarningType::INFO, "This is an informational message.");
    generateWarning(WarningType::WARNING, "This is a warning message.");
    generateWarning(WarningType::ERROR, "This is an error message.");
    generateWarning(); // Function call without required argument
    return 0;
}
