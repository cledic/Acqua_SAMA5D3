The mod_hrt.c file is a very simple module that use high resolution timer.
I need a 14us interval, but for whatever value I use, lower than 60us, I reach a 60us interval.
<br>
The program mod_hrt.c use a pin to toggle up/down whenever the timer handle is called. The mod_hrt2.c use another pin to toggle at the beginning and at the end of the handle.
<br>
Using mod_hrt2.c I can see with the scope that the execution of the handle is 940ns in time.
<br>
The file proc_timer_list.txt is the output of the file: /proc/timer_list. The content of this file confirm thet the system is using the high resolution timer.
