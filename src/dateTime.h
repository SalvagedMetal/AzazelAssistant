#ifndef DATETIME_H
#define DATETIME_H

#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <array>
#include <stdexcept>

namespace DTFormat {
    // Time Formats
    inline const char* HHMMSS24 = "%H:%M:%S";
    inline const char* HHMM24 = "%H:%M";
    inline const char* HHMMSS12 = "%I:%M:%S";
    inline const char* HHMM12 = "%I:%M";

    // Date Formats
    inline const char* DDMMYY = "%d.%m.%y";
    inline const char* DMonthYY = "%e.%B.%y";
    inline const char* DayDMonthYY = "%A.%e.%B.%y";
    inline const char* DDMMYYYY = "%d.%m.%Y";
    inline const char* DMonthYYYY = "%e.%B.%Y";
    inline const char* DayDMonthYYYY = "%A.%e.%B.%Y";
    inline const char* YYYYMMDD = "%Y-%m-%d";
    inline const char* YYYYMMDDHHMMSS = "%Y-%m-%d %H:%M:%S";
}

namespace DateTime {
    extern const std::array<const char*, 8> timeFormats;
    extern const std::array<const char*, 8> dateFormats;

    std::string getCurrentDateTime(const char*);
    std::string getDateTime(const time_t, const char*);
    time_t getCurrentTimestamp();
    time_t getTimestamp(const char*, const char*);
    std::string findFormat(const std::string&);
};

#endif