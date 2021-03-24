#!/bin/sh

# I hate my face, I think I'm ugly, so here we go
./clean.sh

# mjpeg
build/linescapi.demo /dev/video0 output%d.jpeg 5

# rgb24
build/linescapi.demo /dev/video0 output%d.rgb 5 yes