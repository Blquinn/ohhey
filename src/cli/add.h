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

inline void printHelp() {
    std::cerr << helpMessage << std::endl;
    std::exit(1);
}

// The add command takes an image of the user, extracts the persons face descriptors
// and stores them for later use.
inline void run(std::vector<std::string> args) {
    args.erase(args.begin()); // Remove command name.

    if (args.size() != 1) {
        printHelp();
    }

    auto username = args[0];

    auto modelsPath = std::filesystem::current_path().parent_path().append("dlib-data");
    Compare::DlibModels models(modelsPath);

    const int xres = 640;
    const int yres = 480;
    // TODO: Discover devices.
    // Compare::CameraCaptureThread captureThread("/dev/video0", xres, yres);
    Compare::CameraCaptureThread captureThread("/dev/video2", xres, yres);

    auto dims = captureThread.getFrameDimensions();

    dlib::array2d<unsigned char> cameraImage(dims.height, dims.width);
    auto cameraImageWin = std::make_unique<dlib::image_window>(cameraImage);
    cameraImageWin->set_title("Camera image.");

    const auto deadline = std::chrono::system_clock::now() +
                          std::chrono::seconds(10); // TODO: Make configurable timeout.

    bool passedDeadline = false;
    while (!(passedDeadline = std::chrono::system_clock::now() > deadline)) {
        auto currentFrame = captureThread.frame();
        Compare::copyFrameIntoArray2d(cameraImage, currentFrame->data.data());
        cameraImageWin->set_image(cameraImage);
        auto dets = models.getFaceDescriptorsFromImage(cameraImage);

        if (dets.size() != 1) {
            std::clog << "Need 1 detected face in frame, got " << dets.size() << std::endl;
            continue;
        }

        auto face = dets[0];

        std::stringstream path;
        path << "/tmp/" << username << "-desc.dat";

        std::ofstream ofs;
        ofs.open(path.str(), std::ofstream::out | std::ofstream::app);
        dlib::serialize(face, ofs);

        break;
    }
}

} // namespace Add
