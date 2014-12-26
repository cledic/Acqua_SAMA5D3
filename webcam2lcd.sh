#
# To render on the LCD the webcam
#
mplayer -tv driver=v4l2:width=640:height=480:device=/dev/video0 -vf scale=640:480 -vo fbdev2:/dev/fb0 -geometry 640x480+0+0 tv://

# different size...
mplayer -tv driver=v4l2:width=320:height=240:device=/dev/video0 -vf scale=320:240 -vo fbdev2:/dev/fb0 -geometry 320x240+0+0 tv://
