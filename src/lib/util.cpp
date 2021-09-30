#include "util.h"

namespace Util {

Timer::Timer(const std::string &timerName, bool shouldPrint)
    : points(), name(), m_shouldPrint(shouldPrint) {
    if (timerName.empty()) {
        name = "-";
    } else {
        name = timerName;
    }

    // Automatically mark start time.
    mark("start");
}

void Timer::mark(const std::string &pointName, bool compBeginning) {
    Mark m{.name = pointName,
           .time = std::chrono::high_resolution_clock::now(),
           .compBeginning = compBeginning};
    points.push_back(m);
}

void Timer::printTimings() {
    if (!m_shouldPrint)
        return;

    if (points.size() < 2) {
        std::clog << "Not enough timing points to print stats" << std::endl;
        return;
    }

    auto prev = points[0];
    Mark curr;
    for (std::size_t i = 1; i < points.size(); i++) {
        curr = points[i];

        Mark toCompare = curr.compBeginning ? points[0] : prev;

        std::clog << "TIMING(" << name << "): (" << toCompare.name << " -> " << curr.name
                  << ") took "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(curr.time -
                                                                           toCompare.time)
                         .count()
                  << "ms" << std::endl;

        prev = curr;
    }
}
} // namespace Util