#include "linescapi.hpp"
#include <iostream>
#include <fstream>

// capture in 640x480 by default because this is supported by 99.99% of all webcams.
#define WIDTH  (640)
#define HEIGHT (480)

static void dumpToFile(const std::string& path, unsigned char* contents, size_t size) {
    std::cout << "Writing " << size << " bytes to " << path << std::endl;

    std::ofstream out(path, std::ofstream::binary | std::ofstream::out | std::ofstream::trunc);
    out.write(reinterpret_cast<char*>(contents), size);
    out.flush();
    out.close();
}

int main(int argc, char *argv[], char* envp[]) {
    std::cout << "linESCAPI demo by nik" << std::endl;

    if (argc <= 2) {
        std::cout << "Usage:" << std::endl;
        std::cout << "./linescapi.demo <path to video device> <output path>" << std::endl;
        return EXIT_SUCCESS;
    }

    std::string videoDevice(argv[1]);
    std::string outputPath(argv[2]);

    std::cout << "Video device: " << videoDevice << std::endl;
    std::cout << "Output file: " << outputPath << std::endl;

    std::cout << "This demo will only capture frames in the MJPEG format!" << std::endl;

    // a dynamic capture result buffer.
    linescapi::capture_params params(WIDTH, HEIGHT);

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

    // busywait till the camera is doing the job.
    while (!cam.is_capture_done()) {
        /* capture is in progress, just busywait... */
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
    
    // dump to a file.
    dumpToFile(outputPath, params.getData(), params.getLength());
    std::cout << "Capture Done!" << std::endl;
    
    /*  Technically the linescapi::camera's destructor will try to
     *  Automatically end any capture streams *and* close the FD
     *  But that's only as a last resort.
     * 
     *  So please be a nice person, stop the capture and close the camera fd yourself.
     */
    return EXIT_SUCCESS;
}
