#include "compare.h"

namespace Compare {

CliArgs::CliArgs(std::vector<std::string> &argVec) {
    TCLAP::CmdLine cmd("compare is resonsible for comparing the facial "
                       "image to the stored model.",
                       ' ', "0.1");

    TCLAP::UnlabeledValueArg<std::string> userArg("user", "The user to compare to.", true, "",
                                                  "user");
    cmd.add(userArg);

    TCLAP::ValueArg<std::string> deviceArg(
        "D", "device", "The camera device to use e.g. /dev/video0", false, "/dev/video0", "path");
    cmd.add(deviceArg);

    TCLAP::SwitchArg showImageWindowArg("S", "show-image-window",
                                        "Show a debug window for the camera image.", false);
    cmd.add(showImageWindowArg);

    TCLAP::SwitchArg printTimingArg("T", "print-timing", "Print timing stats", false);
    cmd.add(printTimingArg);

    cmd.parse(argVec);

    user = userArg.getValue();
    showImageWindow = showImageWindowArg.getValue();
    printTiming = printTimingArg.getValue();
    device = deviceArg.getValue();
}

std::vector<dlib::matrix<float, 0, 1>>
getFaceDescriptorsFromImage(anet_type &net, const dlib::shape_predictor &sp,
                            dlib::frontal_face_detector &frontalFaceDet,
                            const dlib::array2d<unsigned char> &img) {
    auto dets = frontalFaceDet(img);

    std::vector<dlib::matrix<dlib::rgb_pixel>> faces;

    for (auto &&face : dets) {
        auto shape = sp(img, face);
        dlib::matrix<dlib::rgb_pixel> faceChip;
        dlib::extract_image_chip(img, dlib::get_face_chip_details(shape, 150, 0.25), faceChip);
        faces.push_back(std::move(faceChip));
    }

    return net(faces);
}

void copyFrameIntoArray2d(dlib::array2d<unsigned char> &array, unsigned char *buf) {
    size_t i = 0;
    for (auto &&pix : array) {
        // Convert to greyscale by averaging the rgb values.
        pix = (buf[i] + buf[i + 1] + buf[i + 2]) / 3;

        i += 3;
    }
}

Image::Image(const RGBImage &img) {
    // Copy image data into a vector.
    data = std::vector<unsigned char>(img.data, img.data + img.size);
    width = img.width;
    height = img.height;
    size = img.size;
}

DlibModels::DlibModels(std::filesystem::path basePath, Util::Timer *timer)
    : net(), sp(), frontalFaceDet() {

    auto shapePredictorModelPath = basePath;
    shapePredictorModelPath.append("shape_predictor_5_face_landmarks.dat");

    dlib::deserialize(shapePredictorModelPath) >> sp;

    if (timer)
        timer->mark("load shape predictor");

    frontalFaceDet = dlib::get_frontal_face_detector();

    if (timer)
        timer->mark("load ffd");

    dlib::deserialize(basePath.append("dlib_face_recognition_resnet_model_v1.dat")) >> net;

    if (timer)
        timer->mark("load resnet model");
}

CameraCaptureThread::CameraCaptureThread(const std::string &device, int xres, int yres)
    : t(), m_frame(), cv(), mu(), m_frameDimensions(ImageDims{}), captureThreadDie(false) {

    std::mutex dims_mu;
    std::condition_variable dims_cv;

    t = std::thread([&]() {
        Webcam webcam(device, xres, yres);
        m_frameDimensions = webcam.get_frame_dimensions();

        dims_cv.notify_one();

        while (!captureThreadDie.load()) {
            m_frame = std::make_shared<Image>(webcam.frame());
            cv.notify_one();
        }
    });

    std::unique_lock<std::mutex> lk(dims_mu);
    if (dims_cv.wait_for(lk, std::chrono::seconds(5)) == std::cv_status::timeout) {
        shutdown();
        throw std::runtime_error("Timed out while waiting for frame dimensions.");
    }
}

std::shared_ptr<Image> CameraCaptureThread::frame() {
    std::unique_lock<std::mutex> lk(mu);
    if (cv.wait_for(lk, std::chrono::seconds(5)) == std::cv_status::timeout) {
        throw std::runtime_error("Timed out while waiting for camera frame.");
    }

    return m_frame;
}

void CameraCaptureThread::shutdown() {
    captureThreadDie.store(true);
    t.join();
}

Result run(std::vector<std::string> argVec) {
    try {

        CliArgs args(argVec);

        // Config cnf;

        Util::Timer timer("compare", args.printTiming);

        const int xres = 640;
        const int yres = 480;


        CameraCaptureThread captureThread(args.device, xres, yres);

        timer.mark("create capture thread");

        // TODO: Make configurable.
        std::filesystem::path dlibDataPath("/home/ben/Code/cpp/ohhey/dlib-data");
        DlibModels models(dlibDataPath, &timer);

        timer.mark("load models");

        std::stringstream path;
        path << "/tmp/" << args.user << "-desc.dat";

        dlib::matrix<float, 0, 1> userFaceDescriptor;
        std::ifstream ifs;
        ifs.open(path.str(), std::istream::in);
        dlib::deserialize(ifs) >> userFaceDescriptor;

        timer.mark("load user face descriptor");

        auto frameDims = captureThread.getFrameDimensions();
        dlib::array2d<unsigned char> cameraImage(frameDims.height, frameDims.width);

        std::unique_ptr<dlib::image_window> cameraImageWin;

        if (args.showImageWindow) {
            cameraImageWin = std::make_unique<dlib::image_window>(cameraImage);
            cameraImageWin->set_title("Camera image.");
            timer.mark("launch camera image window");
        }

        const auto deadline = std::chrono::system_clock::now() +
                              std::chrono::seconds(10); // TODO: Make configurable timeout.

        int attempts = 1;
        bool passedDeadline = false;
        while (!(passedDeadline = std::chrono::system_clock::now() > deadline)) {
            // This iterates through every frame captured by the camera.
            // How to make it so we just grab the current frame being captured?
            auto currentFrame = captureThread.frame();
            copyFrameIntoArray2d(cameraImage, currentFrame->data.data());

            if (args.showImageWindow)
                cameraImageWin->set_image(cameraImage);

            auto cameraFaceDescriptors = models.getFaceDescriptorsFromImage(cameraImage);

            timer.mark("get camera face descriptors.");

            // std::clog << "Found " << cameraFaceDescriptors.size() << " faces in camera image."
            //           << std::endl;

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

            attempts++;
        }

        if (passedDeadline) {
            std::clog << "Surpassed deadline while trying to detect matching face after "
                      << attempts << " attempts." << std::endl;
        }

        return Result::ERROR;
    } catch (std::exception &e) {
        std::clog << "Exception occurred during facial recognition: " << e.what() << std::endl;
        return Result::ERROR;
    }
}
} // namespace Compare
