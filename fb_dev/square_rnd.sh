#!/bin/bash

/root/fb_dev/fbtst -c -r 0 -g 0 -b0

while true;
do
        x=$((RANDOM%400+1))
        y=$((RANDOM%240+1))
        w=$((RANDOM%400+1))
        h=$((RANDOM%240+1))
        r=$((RANDOM%250+1))
        g=$((RANDOM%250+1))
        b=$((RANDOM%250+1))

        /root/fb_dev/fbtst -c -x $x -y $y -w $w -h $h -r $r -g $g -b $b
done
