#pragma once

#include <dlib/gui_widgets/widgets.h>
#include <dlib/image_loader/load_image.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/lib/compare.h"

namespace Add {

const static std::string helpMessage = R"(
Usage add [username]
)";

void printHelp();

// The add command takes an image of the user, extracts the persons face descriptors
// and stores them for later use.
void run(std::vector<std::string> args);

} // namespace Add
