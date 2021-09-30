#pragma once

#include "chrono"
#include "iostream"
#include "string"
#include "vector"

namespace Util {

/**
 * Timer is a convenience class for timing stuff in the code.
 */
class Timer {

private:
    typedef std::chrono::time_point<std::chrono::system_clock,
                                    std::chrono::duration<long, std::ratio<1, 1000000000>>>
        time_point;

    struct Mark {
        std::string name;
        time_point time;
        bool compBeginning;
    };

public:
    Timer(const std::string &timerName, bool shouldPrint);

    Timer(bool shouldPrint) : Timer("", shouldPrint) {}

    ~Timer() {
        mark("end");
        printTimings();
    }

    void mark(const std::string &pointName, bool compBeginning = false);

private:
    void printTimings();

    std::vector<Mark> points;
    std::string name;
    bool m_shouldPrint;
};

} // namespace Util
