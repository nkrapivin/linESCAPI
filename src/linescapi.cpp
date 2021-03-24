#include <linux/videodev2.h>
#include <libv4l2.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "linescapi.hpp"

#define CLEAR(x) memset(&(x), 0, (sizeof((x))))

namespace linescapi {
    static bool xioctl(int fh, int request, void *arg) {
        int r; // return code.

        do {
            r = v4l2_ioctl(fh, request, arg);
        } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

        if (r == -1) {
            // an error.
            return false;
        }

        return true;
    }

    capture_params::capture_params() {
        width = 0;
        height = 0;
        length = 0;
        data = nullptr;
        imgformat = V4L2_PIX_FMT_MJPEG;
    }

    capture_params::capture_params(int w, int h, unsigned int format) {
        width = w;
        height = h;
        length = 0;
        data = nullptr;
        imgformat = format;
    }

    capture_params::~capture_params() {
        width = 0;
        height = 0;
        length = 0;

        if (data != nullptr) {
            delete[] data;
            data = nullptr;
        }
    }

    const int capture_params::getWidth() const {
        return width;
    }

    const int capture_params::getHeight() const {
        return height;
    }

    const unsigned int capture_params::getFormat() const {
        return imgformat;
    }

    unsigned char* capture_params::getData() const {
        return data;
    }

    const unsigned int capture_params::getLength() const {
        return length;
    }

    void capture_params::ensure(unsigned int newsize) {
        // reallocate the camera buffer.
        if (newsize != length) {
            if (data != nullptr) {
                std::cout << "[linESCAPI]: " << "Freeing previous dynamic buffer len=" << length << std::endl;
                delete[] data;
            }

            std::cout << "[linESCAPI]: " << "Reallocating buffer to " << newsize << std::endl;
            data = new unsigned char[newsize];
            length = newsize;
        }
    }

    void capture_params::zero() const {
        if (data != nullptr) {
            memset(data, '\0', length);
        }
    }

    const unsigned int capture_params::setFormat(unsigned int format) {
        unsigned int oldF = imgformat;
        imgformat = format;
        return oldF;
    }

    const int capture_params::setWidth(int w) {
        int oldW = width;
        width = w;
        return oldW;
    }

    const int capture_params::setHeight(int h) {
        int oldH = height;
        height = h;
        return oldH;
    }

    camera::camera() {
        fd = -1;
        err = false;
        in_progress = false;
        param = nullptr;
        start = nullptr;
        length = 0;
        CLEAR(fmt);
        CLEAR(req);
        CLEAR(buf);
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }

    bool camera::open(const std::string& path) {
        fd = v4l2_open(path.c_str(), O_RDWR | O_NONBLOCK, 0);
        if (fd < 0) {
            err = true;
            std::cout << "[linESCAPI]: " << "Unable to open camera device." << std::endl;
        }

        return !err;
    }

    bool camera::close() {
        if (fd < 0) {
            return true;
        }

        int r = v4l2_close(fd);
        err = (r == -1);
        if (err) {
            std::cout << "[linESCAPI]: " << "Unable to close camera device." << std::endl;
            return false;
        }

        fd = -1;
        return !err;
    }

    bool camera::is_opened() {
        return (fd >= 0);
    }

    bool camera::good() {
        return !err;
    }

    bool camera::bad() {
        return err;
    }

    bool camera::init_capture(capture_params& cparam) {
        if (in_progress) {
            std::cout << "[linESCAPI]: " << "A capture is already in progress!" << std::endl;
            return false;
        }

        unsigned int imageformat = cparam.getFormat();
        std::cout << "[linESCAPI]: " << "Using image format=" << imageformat << std::endl;
        unsigned int imagefields = V4L2_FIELD_ANY; // technically we shouldn't care if we're using libv4l2...... :/

        CLEAR(fmt);
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = cparam.getWidth();
        fmt.fmt.pix.height = cparam.getHeight();
        fmt.fmt.pix.pixelformat = imageformat;
        fmt.fmt.pix.field = imagefields;
        err = !xioctl(fd, VIDIOC_S_FMT, &fmt);

        if (fmt.fmt.pix.pixelformat != imageformat) {
            std::cout << "[linESCAPI]: " << "Camera does not support the target image format." << std::endl;
            err = true;
        }

        if ((fmt.fmt.pix.width != cparam.getWidth()) || (fmt.fmt.pix.height != cparam.getHeight())) {
            std::cout << "[linESCAPI]: " << "Camera did not like your width or height. Try cropping the resolution in half?" << std::endl;
            err = true;
        }

/*
// err, this caused issues when picking between image formats, so off to the commented code it goes!
        if (fmt.fmt.pix.field != imagefields) {
            std::cout << "[linESCAPI]: " << "Camera does not support target fields. Got " << fmt.fmt.pix.field << std::endl;
            //err = true;
        }
*/
        std::cout << "[linESCAPI]: " << "Camera is using fields=" << fmt.fmt.pix.field << std::endl;

        if (!err) {
            in_progress = true;
            param = &cparam;
        }

        return !err && in_progress;
    }

    bool camera::do_capture() {
        if (!in_progress) {
            std::cout << "[linESCAPI]: " << "There is no capture in progress!" << std::endl;
            return false;
        }

        CLEAR(req);
        req.count = 1;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        err = !xioctl(fd, VIDIOC_REQBUFS, &req);
        if (err) {
            std::cout << "[linESCAPI]: " << "Reqbufs fail." << std::endl;
            return false;
        }

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = 0;
        err = !xioctl(fd, VIDIOC_QUERYBUF, &buf);
        if (err) {
            std::cout << "[linESCAPI]: " << "Querybuf fail." << std::endl;
            return false;
        }

        length = buf.length;
        start = v4l2_mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        err = (start == MAP_FAILED);
        if (err) {
            std::cout << "[linESCAPI]: " << "v4l2_mmap() fail." << std::endl;
            return false;
        }

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = 0;
        err = !xioctl(fd, VIDIOC_QBUF, &buf);
        if (err) {
            std::cout << "[linESCAPI]: " << "QBUF fail." << std::endl;
            return false;
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        err = !xioctl(fd, VIDIOC_STREAMON, &type);
        if (err) {
            std::cout << "[linESCAPI]: " << "STREAMON fail." << std::endl;
            return false;
        }

        std::cout << "[linESCAPI]: " << "mmap'd buffer=" << start << ",len=" << length << std::endl;

        return !err;
    }

    bool camera::is_capture_done() {
        if (!in_progress) {
            std::cout << "[linESCAPI]: " << "There is no capture in progress!" << std::endl;
            return false;
        }

        fd_set fds;
        struct timeval tv;
        int r;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        r = select(fd + 1, &fds, nullptr, nullptr, &tv);
        
        if (r == -1) {
            errno = EINTR;
            return false;
        }

        // Capture done!
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        err = !xioctl(fd, VIDIOC_DQBUF, &buf);
        if (err) {
            std::cout << "[linESCAPI]: " << "DQBUF fail." << std::endl;
            return false;
        }

        // And finally, copy the camera data to our buffer!
        param->ensure(buf.bytesused); // only the actual used amount!
        param->zero(); // just in case.
        memcpy(param->getData(), start, buf.bytesused);

        // just in case, queue the buffer again
        err = !xioctl(fd, VIDIOC_QBUF, &buf);
        if (err) {
            std::cout << "[linESCAPI]: " << "Failed to QBUF buffer after DQBUF." << std::endl;
            return false;
        }

        return !err;
    }

    bool camera::deinit_capture() {
        if (!in_progress) {
            std::cout << "[linESCAPI]: " << "There is no capture in progress!" << std::endl;
            return false;
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        // comment this block of code if you have problems with camera hanging till you replug it o_O
        err = !xioctl(fd, VIDIOC_STREAMOFF, &type);
        if (err) {
            std::cout << "[linESCAPI]: " << "STREAMOFF fail. This may happen with some very cheap webcams! Please reload the uvcvideo module." << std::endl;
            return false;
        }

        err = (v4l2_munmap(start, length) == -1);
        if (err) {
            std::cout << "[linESCAPI]: " << "v4l2_munmap() fail." << std::endl;
            return false;
        }

        in_progress = false;

        return !err;
    }

    camera::~camera() {
        if (in_progress) {
            deinit_capture();
        }

        close();
    }
}