#include "testDateTime.h"


void testGetCurrentDateTime() {
    std::string currentDateTime = DateTime::getCurrentDateTime(DTFormat::YYYYMMDDHHMMSS);
    std::cout << "Current Date and Time: " << currentDateTime << std::endl;
    assert(currentDateTime.size() > 0); // Ensure the string is not empty
}

void testGetDateTime() {
    time_t TimeStamp = 1696159845;
    std::string formattedTime = DateTime::getDateTime(TimeStamp,DTFormat::YYYYMMDDHHMMSS);
    std::cout << "Formatted Current Time: " << formattedTime << std::endl;
    assert(formattedTime.size() > 0); // Ensure the string is not empty
    assert(formattedTime.compare("2023-10-01 12:30:45") == 1); // Check against the expected format
}

void testGetCurrentTimestamp() {
    time_t timestamp = DateTime::getCurrentTimestamp();
    std::string formattedTime = DateTime::getDateTime(timestamp, DTFormat::YYYYMMDDHHMMSS);
    std::cout << "Formatted Timestamp: " << formattedTime << std::endl;
    assert(formattedTime.size() > 0); // Ensure the string is not empty
}

void testGetTimestamp() {
    const char* dateStr = "2023-10-01 12:30:45";
    time_t parsedTimestamp = DateTime::getTimestamp(dateStr, DTFormat::YYYYMMDDHHMMSS);
    std::cout << "Parsed Timestamp: " << parsedTimestamp << std::endl;
    assert(parsedTimestamp != -1); // Ensure the timestamp is valid
    assert(parsedTimestamp == 1696159845); // Check against the expected timestamp
}

void testFindFormat() {
    std::string format = DateTime::findFormat("01.10.2023");
    std::cout << "Detected Format: " << format << std::endl;
    assert(std::strcmp(format.c_str(), DTFormat::DDMMYYYY)); // Check against the expected format
}

