#pragma once
#ifndef _LINESCAPI_H_
#define _LINESCAPI_H_

#include <string>
#include <linux/videodev2.h>

namespace linescapi {
    class capture_params {
        int width;
        int height;
        unsigned int imgformat;
        unsigned int length;
        unsigned char* data;
    public:
        capture_params();
        capture_params(int w, int h, unsigned int format = V4L2_PIX_FMT_MJPEG);
        ~capture_params();

        const int getWidth() const;
        const int getHeight() const;
        const unsigned int getFormat() const;
        unsigned char* getData() const;
        const unsigned int getLength() const;
        const unsigned int setFormat(unsigned int format);
        const int setWidth(int width);
        const int setHeight(int height);

        // only used by the `camera` class.
        void ensure(unsigned int newsize);
        void zero() const;

        // a little conversion helper.
        bool convertToRGBA();
    };

    class camera {
        // internal v4l2 stuff.
        struct v4l2_format fmt;
        struct v4l2_requestbuffers req;
        struct v4l2_buffer buf;
        enum v4l2_buf_type type;

        // camera fd
        int fd;

        // v4l2 mapped buffer
        unsigned int length;
        void* start;

        capture_params* param;

        // flags.
        bool err;
        bool in_progress;
    public:
        camera();
        ~camera();
        bool open(const std::string& path);
        bool close();
        bool is_opened();
        bool good();
        bool bad();

        bool init_capture(capture_params& param);
        bool do_capture();
        bool is_capture_done();
        bool deinit_capture();
        bool name(std::string& result);
    };
}

#endif /* _LINESCAPI_H_ */
