#include "linescapi.hpp"
#include <iostream>
#include <fstream>
#include <unistd.h>

// capture in 640x480 by default because this is supported by 99.99% of all webcams.
#define WIDTH  (640)
#define HEIGHT (480)

#define FOCUS_SLEEP (false)

static void dumpToFile(const std::string& path, unsigned char* contents, size_t size) {
    std::cout << "Writing " << size << " bytes to " << path << std::endl;

    std::ofstream out(path, std::ofstream::binary | std::ofstream::out | std::ofstream::trunc);
    out.write(reinterpret_cast<char*>(contents), size);
    out.flush();
    out.close();
}

int main(int argc, char *argv[], char* envp[]) {
    std::cout << "linESCAPI demo by nik" << std::endl;

    if (argc <= 3) {
        std::cout << "Usage:" << std::endl;
        std::cout << argv[0] << " <path to video device> <output path> <number of frames> [optional]<use RGB? (any word or number)>" << std::endl;
        return EXIT_SUCCESS;
    }

    std::string videoDevice(argv[1]);
    std::string outputPath(argv[2]);
    int framesNum = atoi(argv[3]);
    bool useMJPEG = argc < 5;

    std::cout << "Video device: " << videoDevice << std::endl;
    std::cout << "Output file: " << outputPath << std::endl;

    if (useMJPEG) {
        std::cout << "Will use MJPEG" << std::endl;
    }
    else {
        std::cout << "Will use raw RGB" << std::endl;
    }

    // a dynamic capture result buffer.
    linescapi::capture_params params(WIDTH, HEIGHT, (useMJPEG ? V4L2_PIX_FMT_MJPEG : V4L2_PIX_FMT_RGB24));

    // the camera object.
    linescapi::camera cam;

    // open camera fd
    if (!cam.open(videoDevice)) {
        std::cout << "Failed to open camera device." << std::endl;
        return EXIT_FAILURE;
    }

    // setup width and height.
    if (!cam.init_capture(params)) {
        std::cout << "Failed to initialize capture." << std::endl;
        return EXIT_FAILURE;
    }

    // allocate capture buffers.
    if (!cam.do_capture()) {
        std::cout << "Failed to queue capture buf." << std::endl;
        return EXIT_FAILURE;
    }

    // this is so the camera can focus before giving us the frames.
    if (FOCUS_SLEEP) {
        std::cout << "Giving the camera time to focus..." << std::endl;
        sleep(5);
        std::cout << "Wah, that was a good nap for sure" << std::endl;
    }

    // busywait till the camera is doing the job.
    for (int i = 0; i < framesNum; i++) {
        while (!cam.is_capture_done()) {
            /* capture is in progress, just busywait... */
        }

        // uh oh. continuing at this point is a very bad idea, let's just break.
        if (cam.bad()) break;

        char name[64]{'\0'};
        snprintf(name, sizeof(name)-1, outputPath.c_str(), 1 + i);

        std::cout << "Saving frame num=" << 1+i << std::endl;
        dumpToFile(name, params.getData(), params.getLength());
    }

    // stop the capture stream.
    if (!cam.deinit_capture()) {
        std::cout << "Failed to properly deinit the capture process." << std::endl;
        return EXIT_FAILURE;
    }

    // close the camera fd
    if (!cam.close()) {
        std::cout << "Failed to close the camera FD." << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "Capture Done!" << std::endl;
    
    /*  Technically the linescapi::camera's destructor will try to
     *  Automatically end any capture streams *and* close the FD
     *  But that's only as a last resort.
     * 
     *  So please be a nice person, stop the capture and close the camera fd yourself.
     */
    return EXIT_SUCCESS;
}
