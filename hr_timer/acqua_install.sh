#!/bin/sh
make -C ~/linux-3.10-acqua ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- M=`pwd` modules
sshpass -p acmesystems scp ledpanel3v3.ko root@acqua-ip-address:/root
sshpass -p acmesystems scp mod_hrt.ko root@acqua-ip-address:/root
