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
    inline const char* Day = "%A";
}

namespace DateTime {
    extern const std::array<const char*, 9> timeFormats;
    extern const std::array<const char*, 9> dateFormats;

    /*  
        Gets the current date/time formatted as per the given format.
        const char* format   || Format string
        Returns              || Formatted date/time string.
    */
    std::string getCurrentDateTime(const char* format);
    /*  
        Formats the given timestamp as per the given format.
        const time_t timeStamp  || Timestamp to format
        const char* format      || Format string
        Returns                 || Formatted date/time string.
    */
    std::string getDateTime(const time_t timeStamp, const char* format);
    /*  
        Gets the current timestamp.
        Returns || Current timestamp.
    */
    time_t getCurrentTimestamp();
    /*  
        Parses the given date/time string as per the given format and returns the timestamp.
        const char* dateTime   || Date/time string to parse
        const char* format     || Format string
        Returns                || Parsed timestamp.
    */
    time_t getTimestamp(const char* dateTime, const char* format);
    /*  
        Finds the matching format for the given date/time input string.
        const std::string& input   || Date/time input string
        Returns                    || Matching format string.
    */
    std::string findFormat(const std::string& input);
};

#endif
