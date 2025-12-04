#include "dateTime.h"

const std::array<const char*, 9> DateTime::timeFormats = {
        DTFormat::HHMMSS24,
        DTFormat::HHMM24,
        DTFormat::HHMMSS12,
        DTFormat::HHMM12
    };

const std::array<const char*, 9> DateTime::dateFormats = {
        DTFormat::DDMMYY,
        DTFormat::DMonthYY,
        DTFormat::DayDMonthYY,
        DTFormat::DDMMYYYY,
        DTFormat::DMonthYYYY,
        DTFormat::DayDMonthYYYY,
        DTFormat::YYYYMMDD,
        DTFormat::YYYYMMDDHHMMSS,
        DTFormat::Day
    };

std::string DateTime::getCurrentDateTime(const char* format) {
    return DateTime::getDateTime(time(NULL), format);
}

std::string DateTime::getDateTime(const time_t timeStamp, const char* format) {
    char output[50];
    struct tm datetime = *localtime(&timeStamp);
    if (strftime(output, 50, format, &datetime) == 0) {
        throw std::runtime_error("Failed to format date/time");
    }
    return std::string(output);
}

time_t DateTime::getCurrentTimestamp() {
    time_t timeStamp = time(NULL);
    if (timeStamp == -1) {
        throw std::runtime_error("Failed to get current timestamp");
    }
    return timeStamp;
}

time_t DateTime::getTimestamp(const char* dateTime, const char* format) {
    std::tm tm = {};
    std::istringstream ss(dateTime);
    ss >> std::get_time(&tm, format);
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse date/time");
    }
    return mktime(&tm);
}

std::string DateTime::findFormat(const std::string& input) {
    std::tm tm = {};
    std::istringstream ss;
    bool isDate = input.find('.') != std::string::npos;

    const auto& formats = isDate ? DateTime::dateFormats : DateTime::timeFormats;

    // Iterate through the formats to find a match
    for (const auto& dtfMatch : formats) {
        //clear the stringstream
        ss.str(input);
        ss.clear();

        ss >> std::get_time(&tm, dtfMatch);
        if (!ss.fail()) {
            return dtfMatch;
        }
    }
    throw std::runtime_error("No matching format found for input: " + input);
}
