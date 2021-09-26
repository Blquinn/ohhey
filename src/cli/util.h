#pragma once

#include "chrono"
#include "string"
#include "iostream"
#include "vector"

namespace Util {

/**
 * Timer is a convenience class for timing stuff in the code.
 */
class Timer {

private:

typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<long, std::ratio<1, 1000000000>>> time_point;

    struct Mark {
        std::string name;
        time_point time;
        bool compBeginning;
    };

public:
    Timer(const std::string &timerName) 
        : points()
        , name() {
        if (timerName.empty()) {
            name = "-";
        } else {
            name = timerName;
        }

        // Automatically mark start time.
        markTime("start");
    }

    Timer() : Timer("") {
    }

    ~Timer() {
        std::cerr << "Printing timing data." << std::endl;
        markTime("end");
        printTimings();
    }

    void markTime(const std::string &pointName, bool compBeginning = false) {
        Mark m { 
            .name = pointName, 
            .time = std::chrono::high_resolution_clock::now(), 
            .compBeginning = compBeginning 
        };
        points.push_back(m);
    }

private:
    void printTimings() {
        if (points.size() < 2) {
            std::clog << "Not enough timing points to print stats" << std::endl;
            return;
        }

        auto prev = points[0];
        Mark curr;
        for (std::size_t i = 1; i < points.size(); i++) {
            curr = points[i];

            Mark toCompare = curr.compBeginning ? points[0] : prev;

            std::clog << "TIMING(" << name << "): (" << toCompare.name<< " -> " << curr.name 
            << ") took " 
            << std::chrono::duration_cast<std::chrono::milliseconds>(curr.time - toCompare.time).count()
            << "ms" << std::endl;

            prev = curr;
        }
    }

    std::vector<Mark> points;
    std::string name;
};

}
