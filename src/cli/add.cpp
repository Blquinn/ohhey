#include "add.h"
#include <filesystem>
#include <unistd.h>

namespace Add {

void printHelp() {
    std::cerr << helpMessage << std::endl;
    std::exit(1);
}

void checkIsRoot() {
    if (getuid()) {
        std::clog << "You need to be the root user to run add." << std::endl;
        std::exit(2);
    }
}

void run(std::vector<std::string> args) {
    args.erase(args.begin()); // Remove command name.

    if (args.size() != 1) {
        printHelp();
    }

    checkIsRoot();

    auto username = args[0];

    auto modelsPath = std::filesystem::path("/usr/local/share/ohhey");
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

        Util::createDataDir();

        std::ofstream ofs;
        ofs.open(Util::getFacePath(username), std::ofstream::out | std::ofstream::app);
        dlib::serialize(face, ofs);

        break;
    }
}

} // namespace Add