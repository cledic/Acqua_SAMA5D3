gcc -O3 -c gpioFastLib2.c
gcc -O3 -o gpio_tst fastlib_tst.c gpioFastLib2.o
gcc -O3 -o ledpanel ledpanel_exe.c gpioFastLib2.o
