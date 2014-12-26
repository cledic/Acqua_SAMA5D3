#
# Command to play a movi an the LCD
#
mplayer -cache 200000 -nolirc -vo fbdev2:/dev/fb0 -nosound -lavdopts fast:skipframe=nonref:skiploopfilter=nonref -hardframedrop -geometry 426x240+0+0 Video/timelapse2.mp4

#
mplayer -cache 200000 -nolirc -vo fbdev2:/dev/fb0 -nosound -lavdopts fast:skipframe=nonref:skiploopfilter=nonref -hardframedrop big_buck_acqua5_RF16.mp4
