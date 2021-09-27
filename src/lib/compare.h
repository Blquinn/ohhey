#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlib/data_io/load_image_dataset.h>
#include <dlib/gui_widgets/widgets.h>
#include <dlib/image_saver/image_saver.h>
#include <dlib/image_saver/save_jpeg.h>
#include <exception>
#include <filesystem>
#include <iostream>
#include <linux/videodev2.h>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <sys/ioctl.h>
#include <tclap/SwitchArg.h>
#include <tclap/ValueArg.h>
#include <thread>
#include <type_traits>
#include <vector>

#include "tclap/CmdLine.h"
#include <dlib/dnn.h>
#include <dlib/image_loader/load_image.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/shape_predictor.h>

#include "src/lib/webcam-v4l2/webcam.h"

#include "src/lib/defer.h"

#include "src/cli/cli.h"
#include "src/lib/util.h"

static std::string facesDir = "/home/ben/Code/cpp/dlib/examples/faces";

namespace Compare {

struct CliArgs {
    CliArgs(std::vector<std::string> &argVec) {
        TCLAP::CmdLine cmd("compare is resonsible for comparing the facial "
                           "image to the stored model.",
                           ' ', "0.1");

        // TCLAP::UnlabeledValueArg<std::string> userArg(
        //     "user", "The user to compare to.", true, "", "user");
        // cmd.add(userArg);

        TCLAP::UnlabeledValueArg<std::string> userImageArg(
            "userImage", "The image with the stored face.", true, "", "userImage");
        cmd.add(userImageArg);

        TCLAP::SwitchArg showImageWindowArg("S", "show-image-window",
                                            "Show a debug window for the camera image.", false);
        cmd.add(showImageWindowArg);

        cmd.parse(argVec);

        // user = userArg.getValue();
        userImage = userImageArg.getValue();
        showImageWindow = showImageWindowArg.getValue();
    }

    // std::string user;
    std::string userImage;
    bool showImageWindow;
};

// struct Config {
//     Config() {
//         // std::string file = "/etc/ohhey/config.ini";
//         std::string file =
//             std::filesystem::current_path().parent_path().append("etc").append("config.ini");

//         INIReader reader(file);
//         if (reader.ParseError() < 0) {
//             throw "Failed to read config file.";
//         }

//         useCnn = reader.GetBoolean("core", "use_cnn", false);
//     }

//     bool useCnn;
// };

/////////////////////////////////
// dlib templates (sourced from http://dlib.net/dnn_face_recognition_ex.cpp.html)

template <template <int, template <typename> class, int, typename> class block, int N,
          template <typename> class BN, typename SUBNET>
using residual = dlib::add_prev1<block<N, BN, 1, dlib::tag1<SUBNET>>>;

template <template <int, template <typename> class, int, typename> class block, int N,
          template <typename> class BN, typename SUBNET>
using residual_down = dlib::add_prev2<
    dlib::avg_pool<2, 2, 2, 2, dlib::skip1<dlib::tag2<block<N, BN, 2, dlib::tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET>
using block =
    BN<dlib::con<N, 3, 3, 1, 1, dlib::relu<BN<dlib::con<N, 3, 3, stride, stride, SUBNET>>>>>;

template <int N, typename SUBNET> using ares = dlib::relu<residual<block, N, dlib::affine, SUBNET>>;
template <int N, typename SUBNET>
using ares_down = dlib::relu<residual_down<block, N, dlib::affine, SUBNET>>;

template <typename SUBNET> using alevel0 = ares_down<256, SUBNET>;
template <typename SUBNET> using alevel1 = ares<256, ares<256, ares_down<256, SUBNET>>>;
template <typename SUBNET> using alevel2 = ares<128, ares<128, ares_down<128, SUBNET>>>;
template <typename SUBNET> using alevel3 = ares<64, ares<64, ares<64, ares_down<64, SUBNET>>>>;
template <typename SUBNET> using alevel4 = ares<32, ares<32, ares<32, SUBNET>>>;

using anet_type = dlib::loss_metric<dlib::fc_no_bias<
    128, dlib::avg_pool_everything<alevel0<alevel1<alevel2<alevel3<alevel4<dlib::max_pool<
             3, 3, 2, 2,
             dlib::relu<dlib::affine<
                 dlib::con<32, 7, 7, 2, 2, dlib::input_rgb_image_sized<150>>>>>>>>>>>>>;

/////////////////////////////////
// end dlib templates

inline std::vector<dlib::matrix<float, 0, 1>>
getFaceDescriptorsFromImage(anet_type &net, const dlib::shape_predictor &sp,
                            dlib::frontal_face_detector &frontalFaceDet,
                            const dlib::array2d<unsigned char> &img) {
    auto dets = frontalFaceDet(img);

    std::vector<dlib::matrix<dlib::rgb_pixel>> faces;

    for (auto face : dets) {
        auto shape = sp(img, face);
        dlib::matrix<dlib::rgb_pixel> faceChip;
        dlib::extract_image_chip(img, dlib::get_face_chip_details(shape, 150, 0.25), faceChip);
        faces.push_back(std::move(faceChip));
    }

    return net(faces);
}

inline void copyFrameIntoArray2d(dlib::array2d<unsigned char> &array, unsigned char *buf) {
    size_t i = 0;
    for (auto &&pix : array) {
        // Convert to greyscale by averaging the rgb values.
        pix = (buf[i] + buf[i + 1] + buf[i + 2]) / 3;

        i += 3;
    }
}

struct Image {
    std::vector<unsigned char> data; // RGB888 <=> RGB24
    size_t width;
    size_t height;
    size_t size; // width * height * 3

    Image(const RGBImage &img) {
        // Copy image data into a vector.
        data = std::vector<unsigned char>(img.data, img.data + img.size);
        width = img.width;
        height = img.height;
        size = img.size;
    }
};

struct Frame {
public:
    void setFrame(const RGBImage f) {
        std::lock_guard<std::mutex> _lock(m_mu);

        // Copy frame data.
        m_frame = std::make_shared<Image>(f);
    }

    std::shared_ptr<Image> frame() {
        std::lock_guard<std::mutex> _lock(m_mu);

        return m_frame;
    }

private:
    std::shared_ptr<Image> m_frame;
    std::mutex m_mu;
};

enum class Result { AUTH_SUCCESS, AUTH_FAIULRE, ERROR };

inline Result run(std::vector<std::string> argVec) {
    try {

        CliArgs args(argVec);

        std::clog << "Running compare of " << args.userImage << " vs camera image" << std::endl;

        // Config cnf;

        Util::Timer timer("compare");

        auto dlibDataPath = std::filesystem::current_path().parent_path().append("dlib-data");

        auto shapePredictorModelPath = dlibDataPath;
        shapePredictorModelPath.append("shape_predictor_5_face_landmarks.dat");

        dlib::shape_predictor sp;
        dlib::deserialize(shapePredictorModelPath) >> sp;

        timer.mark("load shape predictor");

        dlib::frontal_face_detector frontalFaceDet = dlib::get_frontal_face_detector();

        timer.mark("load ffd");

        anet_type net;
        dlib::deserialize(dlibDataPath.append("dlib_face_recognition_resnet_model_v1.dat")) >> net;

        timer.mark("load resnet model");

        dlib::array2d<unsigned char> userImage;
        dlib::load_image(userImage, args.userImage);
        auto userFaceDescriptors = getFaceDescriptorsFromImage(net, sp, frontalFaceDet, userImage);

        // dlib::image_window userImageWin(userImage);
        // userImageWin.set_title("User image.");

        timer.mark("get user face descriptors.");

        if (userFaceDescriptors.size() != 1) {
            std::cerr << "Expected only 1 face descriptor for user image, got "
                      << userFaceDescriptors.size() << std::endl;
            return Result::ERROR;
        }

        auto userFaceDescriptor = userFaceDescriptors[0];

        // TODO: Figure out how to get higher res images.
        const int xres = 640;
        const int yres = 480;

        Frame frame;
        std::condition_variable cv;
        std::mutex mu;
        std::atomic<bool> captureThreadDie(false);

        // We need to continuously read frames from v4l2, otherwise it will receive
        // frames in order that they were captured. We always want to take the most recent
        // frame, so that the slowness of the recognition doesn't create a backlog of frames.
        // TODO: Optimize frame rate request.
        std::thread captureThread([&]() {
            Webcam webcam("/dev/video0", xres, yres);
            // Webcam webcam("/dev/video2", xres, yres);

            while (!captureThreadDie.load()) {
                frame.setFrame(webcam.frame());
                cv.notify_one();
            }
        });

        defer({
            // Kill video capture thread.
            captureThreadDie.store(true);
            captureThread.join();
        });

        { // Wait for webcam to capture one frame.
            std::unique_lock<std::mutex> lk(mu);
            cv.wait_for(lk, std::chrono::seconds(5));
        }

        dlib::array2d<unsigned char> cameraImage(yres, xres);

        std::unique_ptr<dlib::image_window> cameraImageWin;

        if (args.showImageWindow) {
            cameraImageWin = std::make_unique<dlib::image_window>(cameraImage);
            cameraImageWin->set_title("Camera image.");
        }

        const auto deadline = std::chrono::system_clock::now() +
                              std::chrono::seconds(10); // TODO: Make configurable timeout.

        std::shared_ptr<Image> currentFrame;
        bool passedDeadline = false;
        while (!(passedDeadline = std::chrono::system_clock::now() > deadline)) {
            // This iterates through every frame captured by the camera.
            // How to make it so we just grab the current frame being captured?
            currentFrame = frame.frame();
            copyFrameIntoArray2d(cameraImage, currentFrame->data.data());

            if (args.showImageWindow)
                cameraImageWin->set_image(cameraImage);

            auto cameraFaceDescriptors =
                getFaceDescriptorsFromImage(net, sp, frontalFaceDet, cameraImage);

            timer.mark("get camera face descriptors.");

            std::clog << "Found " << cameraFaceDescriptors.size() << " faces in camera image."
                      << std::endl;

            // We can check to see if any face on the camera is similar to the users face.
            for (auto &&cfd : cameraFaceDescriptors) {
                // Explanation for 0.6 is found here:
                // http://dlib.net/dnn_face_recognition_ex.cpp.html
                // TODO: Experiment with other values?
                // TODO: Make configurable.
                if (length(userFaceDescriptor - cfd) < 0.6) {
                    std::clog << "Found matching face in camera image." << std::endl;
                    return Result::AUTH_SUCCESS;
                }
            }

            std::clog << "Failed to find matching face in camera image." << std::endl;
        }

        if (passedDeadline) {
            std::clog << "Surpassed deadline while trying to detect matching face." << std::endl;
        }

        return Result::ERROR;
    } catch (std::exception &e) {
        std::clog << "Exception occurred during facial recognition: " << e.what() << std::endl;
        return Result::ERROR;
    }
}

} // namespace Compare
