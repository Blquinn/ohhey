#pragma once

#include "chrono"
#include "filesystem"
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

// TODO: Review permissions of this directory.
// TODO: Make configurable.
static const std::string DATA_DIR = "/usr/local/share/ohhey";
static const std::string FACES_DATA_DIR = DATA_DIR + "/faces";

inline void createDataDir() {
    std::filesystem::create_directories(DATA_DIR);
    std::filesystem::create_directories(FACES_DATA_DIR);
}

inline std::string getFacePath(std::string user) {
    return FACES_DATA_DIR + "/" + user + "-desc.dat";
}

} // namespace Util
