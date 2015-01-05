#include <linux/init.h> 
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/irqflags.h>

#include <asm/io.h>
#include <mach/at91_pio.h>
#include <mach/hardware.h>

#define PB_SODR AT91_IO_P2V(0xFFFFF400+0x30) // PB SODR 
#define PB_CODR AT91_IO_P2V(0xFFFFF400+0x34) // PB CODR 

#define TP_MASK		(1 << 11)

#define TP_HI	*((unsigned long *)PB_SODR) = (unsigned long)(TP_MASK);  
#define TP_LO	*((unsigned long *)PB_CODR) = (unsigned long)(TP_MASK);  

unsigned long timer_interval_ns = 90*1000;
static struct hrtimer hr_timer;
static unsigned int cnt;

MODULE_LICENSE("Dual BSD/GPL");
MODULE_LICENSE("GPL");

MODULE_AUTHOR("Clemente di Caprio");
MODULE_DESCRIPTION("Simple HRT  module for time testing");

enum hrtimer_restart timer_callback( struct hrtimer *timer_for_restart )
{
	
  	ktime_t currtime , interval;

	TP_HI;

  	currtime  = ktime_get();
  	interval = ktime_set(0,timer_interval_ns);
	
#if 0
  	hrtimer_forward(timer_for_restart, currtime , interval);
#else
  	hrtimer_forward(timer_for_restart, hrtimer_cb_get_time( timer_for_restart), interval);
#endif

#if 0
#if 0
	gpio_set_value( 43,(cnt++ & 1)); //Toggle LED 
#else
	if ( cnt++ & 1) {
		TP_HI;
	} else {
		TP_LO
	}
#endif
#endif

	TP_LO;

	return HRTIMER_RESTART;
}


static int __init timer_init(void) 
{
	ktime_t ktime;
	int rtc;
	struct timespec tp;
	
	rtc=gpio_request( 43,"TP");
	if (rtc!=0) return -1;
	rtc=gpio_direction_output(43,0);
	if (rtc!=0) return -1;

	gpio_set_value( 43, 0);
  	printk(KERN_INFO "HR Timer module OK gpio\n");

	hrtimer_get_res(CLOCK_MONOTONIC, &tp);
	printk(KERN_INFO "Clock resolution: %ldns\n", tp.tv_nsec);
	
	ktime = ktime_set( 0, timer_interval_ns );
	hrtimer_init( &hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	hr_timer.function = &timer_callback;
 	hrtimer_start( &hr_timer, ktime, HRTIMER_MODE_REL );

	return 0;
}

static void __exit timer_exit(void) {
	int ret;
	
	gpio_free( 43);
  	ret = hrtimer_cancel( &hr_timer );
  	if (ret) printk("The timer was still in use...\n");
  	printk("HR Timer module uninstalling\n");
	
}

module_init(timer_init);
module_exit(timer_exit);

