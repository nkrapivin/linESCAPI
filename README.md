# linESCAPI
Depends on libv4l2 in order to work. :/

The most simplistic code example:
```cpp
int w = 640, h = 480;
linescapi::capture_params params(w, h);

linescapi::camera cam;

cam.open("/dev/video0"); // make your own enumeration function if you care.
cam.init_capture(params);
cam.do_capture();

while (!cam.is_capture_done()) {
    /* using usleep or other ways to not busywait may introduce slight timing issues... */
}

cam.deinit_capture();
cam.close();


unsigned char* image = params.getData();
size_t imagelen = params.getLength();
// and now use ofstream or any other means of dumping memory to a file...
```
