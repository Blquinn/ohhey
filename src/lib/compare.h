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
#include <optional>
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
    CliArgs(std::vector<std::string> &argVec);

    std::string user;
    std::string device;
    bool showImageWindow;
    bool printTiming;
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

std::vector<dlib::matrix<float, 0, 1>>
getFaceDescriptorsFromImage(anet_type &net, const dlib::shape_predictor &sp,
                            dlib::frontal_face_detector &frontalFaceDet,
                            const dlib::array2d<unsigned char> &img);

void copyFrameIntoArray2d(dlib::array2d<unsigned char> &array, unsigned char *buf);

struct Image {
    std::vector<unsigned char> data; // RGB888 <=> RGB24
    size_t width;
    size_t height;
    size_t size; // width * height * 3

    Image(const RGBImage &img);
};

struct DlibModels {
    anet_type net;
    dlib::shape_predictor sp;
    dlib::frontal_face_detector frontalFaceDet;

    DlibModels(std::filesystem::path basePath, Util::Timer *timer = nullptr);

    std::vector<dlib::matrix<float, 0, 1>>

    getFaceDescriptorsFromImage(const dlib::array2d<unsigned char> &img) {
        return Compare::getFaceDescriptorsFromImage(net, sp, frontalFaceDet, img);
    }
};

struct CameraCaptureThread {
public:
    // TODO: Figure out how to get higher res images.
    CameraCaptureThread(const std::string &device, int xres, int yres);

    ~CameraCaptureThread() { shutdown(); }

    std::shared_ptr<Image> frame();

    ImageDims getFrameDimensions() { return m_frameDimensions; }

private:
    void shutdown();

    std::thread t;
    std::shared_ptr<Image> m_frame;
    std::condition_variable cv;
    std::mutex mu;
    ImageDims m_frameDimensions;
    std::atomic<bool> captureThreadDie;
};

enum class Result { AUTH_SUCCESS, AUTH_FAIULRE, ERROR };

Result run(std::vector<std::string> argVec);

} // namespace Compare
