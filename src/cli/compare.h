#pragma once

#include <algorithm>
#include <chrono>
#include <dlib/array2d/array2d_kernel.h>
#include <dlib/dnn/core.h>
#include <dlib/gui_widgets/widgets.h>
#include <dlib/image_loader/load_image.h>
#include <dlib/image_processing/shape_predictor.h>
#include <dlib/image_transforms/interpolation.h>
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

#include "tclap/CmdLine.h"
#include <dlib/clustering.h>
#include <dlib/dnn.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/string.h>

#include "../lib/INIReader.h"
#include "cli.h"
#include "util.h"

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

        TCLAP::UnlabeledValueArg<std::string> cameraImageArg(
            "cameraImageArg", "THe image with the unknown face.", true, "", "cameraImage");
        cmd.add(cameraImageArg);

        cmd.parse(argVec);

        // user = userArg.getValue();
        userImage = userImageArg.getValue();
        cameraImage = cameraImageArg.getValue();
    }

    // std::string user;
    std::string userImage;
    std::string cameraImage;
};

struct Config {
    Config() {
        // std::string file = "/etc/ohhey/config.ini";
        std::string file =
            std::filesystem::current_path().parent_path().append("etc").append("config.ini");

        INIReader reader(file);
        if (reader.ParseError() < 0) {
            throw "Failed to read config file.";
        }

        useCnn = reader.GetBoolean("core", "use_cnn", false);
    }

    bool useCnn;
};

/////////////////////////////////
// dlib templates

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
getFaceDescriptorsFromImage(anet_type net, dlib::shape_predictor sp,
                            dlib::frontal_face_detector frontalFaceDet, std::string path) {

    dlib::array2d<unsigned char> img;
    dlib::load_image(img, path);

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

void run(std::vector<std::string> &argVec) {
    CliArgs args(argVec);

    std::clog << "Running compare of " << args.userImage << " vs " << args.cameraImage << std::endl;

    // Config cnf;

    Util::Timer timer("compare");

    auto dlibDataPath = std::filesystem::current_path().parent_path().append("dlib-data");

    auto shapePredictorModelPath = dlibDataPath;
    shapePredictorModelPath.append("shape_predictor_5_face_landmarks.dat");

    dlib::shape_predictor sp;
    dlib::deserialize(shapePredictorModelPath) >> sp;

    timer.markTime("load shape predictor");

    dlib::frontal_face_detector frontalFaceDet = dlib::get_frontal_face_detector();

    timer.markTime("load ffd");

    anet_type net;
    dlib::deserialize(dlibDataPath.append("dlib_face_recognition_resnet_model_v1.dat")) >> net;

    timer.markTime("load resnet model");

    auto userFaceDescriptors = getFaceDescriptorsFromImage(net, sp, frontalFaceDet, args.userImage);

    timer.markTime("get user face descriptors.");

    if (userFaceDescriptors.size() != 1) {
        std::cerr << "Expected only 1 face descriptor for user image, got "
                  << userFaceDescriptors.size() << std::endl;
        return;
    }

    auto userFaceDescriptor = userFaceDescriptors[0];

    auto cameraFaceDescriptors =
        getFaceDescriptorsFromImage(net, sp, frontalFaceDet, args.cameraImage);

    timer.markTime("get camera face descriptors.");

    std::clog << "Found " << cameraFaceDescriptors.size() << " faces in camera image." << std::endl;

    // We can check to see if any face on the camera is similar to the users
    // face.
    for (auto &&cfd : cameraFaceDescriptors) {
        if (length(userFaceDescriptor - cfd) < 0.6) {
            std::cerr << "Found matching face in camera image." << std::endl;
            return;
        }
    }

    std::cerr << "Failed to find matching face in camera image." << std::endl;
}

} // namespace Compare
