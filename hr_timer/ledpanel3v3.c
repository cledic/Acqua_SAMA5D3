/*
 * ledpanel2
 * 
 * Bit banging Linux driver for RGB 32x32 led panel
 * Optimized version with brightness control
 * 
 * (c) 2014 Sergio Tanzilli - tanzilli@acmesystems.it 
 * http://www.acmesystems.it/ledpanel 
 * 
 */

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

#define LEDPANEL_R0		0
#define LEDPANEL_G0		1
#define LEDPANEL_B0		2
#define LEDPANEL_R1		3
#define LEDPANEL_G1		4
#define LEDPANEL_B1		5
#define LEDPANEL_A		6
#define LEDPANEL_B		7
#define LEDPANEL_C		8
#define LEDPANEL_D		9
#define LEDPANEL_CLK	10
#define LEDPANEL_STB	11
#define LEDPANEL_OE		12
#define LEDPANEL_TP		13

#define MAXBUFFER_PER_PANEL 32*32*3

MODULE_LICENSE("Dual BSD/GPL");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergio Tanzilli");
MODULE_DESCRIPTION("Bit banging driver for RGB 32x32 LED PANEL");
 
#define TOP_BYTE_ARRAY 		0 
#define BOTTOM_BYTE_ARRAY 	32*16*3 
 
static struct hrtimer hr_timer; 
static int newframe=0;
static int ledpanel_row=0;
static int pbuffer_top=TOP_BYTE_ARRAY;
static int pbuffer_bottom=BOTTOM_BYTE_ARRAY;

static DEFINE_MUTEX(sysfs_lock);
static unsigned char rgb_buffer[MAXBUFFER_PER_PANEL];
static unsigned char rgb_buffer_copy[MAXBUFFER_PER_PANEL];

// 3 bit per color
#define COLOR_MASK  0x07

/* Port assignement */
// Port assigned ID
#define PORTA_ID		(32*0)
#define PORTB_ID		(32*1)
#define PORTC_ID		(32*2)
#define PORTD_ID		(32*3)
#define PORTE_ID		(32*4)

#define PORT_ID		PORTB_ID
/* ******************************** */

/* Port Address SAM5D31 */
#define PORTA_ADDR	0xFFFFF200
#define PORTB_ADDR	0xFFFFF400
#define PORTC_ADDR	0xFFFFF600
#define PORTD_ADDR	0xFFFFF800
#define PORTE_ADDR	0xFFFFFA00

#if PORT_ID == PORTA_ID
	#define PORT_ADDR PORTA_ADDR
#elif PORT_ID == PORTB_ID
	#define PORT_ADDR PORTB_ADDR
#elif PORT_ID == PORTC_ID
	#define PORT_ADDR PORTC_ADDR
#elif PORT_ID == PORTD_ID
	#define PORT_ADDR PORTD_ADDR
#elif PORT_ID == PORTD_ID
	#define PORT_ADDR PORTD_ADDR	
#elif PORT_ID == PORTE_ID
	#define PORT_ADDR PORTE_ADDR	
#endif

#define PB_SODR AT91_IO_P2V(PORT_ADDR+0x30) // PB SODR 
#define PB_CODR AT91_IO_P2V(PORT_ADDR+0x34) // PB CODR 

/* Bit position */
// RGB 0 pin positions
#define R0_PPOS 12
#define G0_PPOS 13
#define B0_PPOS 14 
// RGB 1 pin position
#define R1_PPOS 15
#define G1_PPOS 16
#define B1_PPOS 17
// Row address pin positions MUST BE CONSECUTIVE
#define A_PPOS 25
#define B_PPOS 26
#define C_PPOS 27
#define D_PPOS 28
// Control signal pin positions
#define CLK_PPOS  8 // Clock (low to hi)
#define STB_PPOS  9 // Strobe (low to hi) 
#define OE_PPOS	 10 // Output enable (active low)
// Test point
#define TP_PPOS 29
/* ********************************* */

/* Bit MASK */
// RGB lines for the top side
#define R0_MASK (1 << R0_PPOS)
#define G0_MASK (1 << G0_PPOS)
#define B0_MASK (1 << B0_PPOS)
// RGB lines for the bottom side
#define R1_MASK (1 << R1_PPOS)
#define G1_MASK (1 << G1_PPOS)
#define B1_MASK (1 << B1_PPOS)
// Row address MUST BE CONSECUTIVE
#define A_MASK  (1 << A_PPOS)
#define B_MASK  (1 << B_PPOS)
#define C_MASK  (1 << C_PPOS)
#define D_MASK  (1 << D_PPOS)
// Control signal
#define CLK_MASK	(1 << CLK_PPOS) // Clock (low to hi)
#define STB_MASK	(1 << STB_PPOS) // Strobe (low to hi) 
#define OE_MASK		(1 << OE_PPOS) // Output enable (active low)
// Test point line for timing measurements
#define TP_MASK		(1 << TP_PPOS) 
/* ********************************** */

// GPIO lines used 
static char ledpanel_gpio[] = {
	(R0_PPOS+PORT_ID), // R0
	(G0_PPOS+PORT_ID), // G0
	(B0_PPOS+PORT_ID), // B0 
	 
	(R1_PPOS+PORT_ID), // R1
	(G1_PPOS+PORT_ID), // G1 
	(B1_PPOS+PORT_ID), // B1
	   
	(A_PPOS+PORT_ID), // A
	(B_PPOS+PORT_ID), // B
	(C_PPOS+PORT_ID), // C
	(D_PPOS+PORT_ID), // D
	
	(CLK_PPOS+PORT_ID), // CLK
	(STB_PPOS+PORT_ID), // STB
	(OE_PPOS+PORT_ID), // OE 
	(TP_PPOS+PORT_ID), // TP 
}; 


// Faster way I found to manage a GPIO line
#define R0_HI	*((unsigned long *)PB_SODR) = (unsigned long)(R0_MASK);  
#define R0_LO	*((unsigned long *)PB_CODR) = (unsigned long)(R0_MASK);  
#define G0_HI	*((unsigned long *)PB_SODR) = (unsigned long)(G0_MASK);  
#define G0_LO	*((unsigned long *)PB_CODR) = (unsigned long)(G0_MASK);  
#define B0_HI	*((unsigned long *)PB_SODR) = (unsigned long)(B0_MASK);  
#define B0_LO	*((unsigned long *)PB_CODR) = (unsigned long)(B0_MASK);  

#define R1_HI	*((unsigned long *)PB_SODR) = (unsigned long)(R1_MASK);  
#define R1_LO	*((unsigned long *)PB_CODR) = (unsigned long)(R1_MASK);  
#define G1_HI	*((unsigned long *)PB_SODR) = (unsigned long)(G1_MASK);  
#define G1_LO	*((unsigned long *)PB_CODR) = (unsigned long)(G1_MASK);  
#define B1_HI	*((unsigned long *)PB_SODR) = (unsigned long)(B1_MASK);  
#define B1_LO	*((unsigned long *)PB_CODR) = (unsigned long)(B1_MASK);  

#define A_HI	*((unsigned long *)PB_SODR) = (unsigned long)(A_MASK);  
#define A_LO	*((unsigned long *)PB_CODR) = (unsigned long)(A_MASK);  
#define B_HI	*((unsigned long *)PB_SODR) = (unsigned long)(B_MASK);  
#define B_LO	*((unsigned long *)PB_CODR) = (unsigned long)(B_MASK);  
#define C_HI	*((unsigned long *)PB_SODR) = (unsigned long)(C_MASK);  
#define C_LO	*((unsigned long *)PB_CODR) = (unsigned long)(C_MASK);  
#define D_HI	*((unsigned long *)PB_SODR) = (unsigned long)(D_MASK);  
#define D_LO	*((unsigned long *)PB_CODR) = (unsigned long)(D_MASK);  

#define CLK_HI	*((unsigned long *)PB_SODR) = (unsigned long)(CLK_MASK);  
#define CLK_LO	*((unsigned long *)PB_CODR) = (unsigned long)(CLK_MASK);  
#define STB_HI	*((unsigned long *)PB_SODR) = (unsigned long)(STB_MASK);  
#define STB_LO	*((unsigned long *)PB_CODR) = (unsigned long)(STB_MASK);  
#define OE_HI	*((unsigned long *)PB_SODR) = (unsigned long)(OE_MASK);  
#define OE_LO	*((unsigned long *)PB_CODR) = (unsigned long)(OE_MASK);  

#define TP_HI	*((unsigned long *)PB_SODR) = (unsigned long)(TP_MASK);  
#define TP_LO	*((unsigned long *)PB_CODR) = (unsigned long)(TP_MASK);  


const char *ledpanel_label[] = {
	"R0",
	"G0",
	"B0", 
	"R1",
	"G1", 
	"B1",
	"A",
	"B",
	"C",
	"D",
	"CLK",
	"STB",
	"OE", 
	"TP", 
}; 

// This function is called when you write something on /sys/class/ledpanel/rgb_buffer
// passing in *buf the incoming content

// rgb_buffer is the content to show on the panel(s)
static ssize_t ledpanel_rgb_buffer(struct class *class, struct class_attribute *attr, const char *buf, size_t len) {
	int i;
	mutex_lock(&sysfs_lock);
	
	if ((len<=MAXBUFFER_PER_PANEL)) {
		memset(rgb_buffer,MAXBUFFER_PER_PANEL,0);
		memcpy(rgb_buffer,buf,len);
	} else {
		memcpy(rgb_buffer,buf,MAXBUFFER_PER_PANEL);
	}		

	for (i=0;i<MAXBUFFER_PER_PANEL;i++) {
		rgb_buffer[i]>>=5;
		rgb_buffer[i]&=COLOR_MASK;
	}	

	memcpy(rgb_buffer_copy,rgb_buffer,MAXBUFFER_PER_PANEL);
	newframe=1;
	
	mutex_unlock(&sysfs_lock);
	//printk(KERN_INFO "Buffer len %d bytes\n", len);
	
	/*for (i=BOTTOM_BYTE_ARRAY;i<BOTTOM_BYTE_ARRAY+30;i+=3) {
		printk(KERN_INFO "Banco 2: %02X %02X %02X\n",rgb_buffer[i+0],rgb_buffer[i+1],rgb_buffer[i+2]);
	}*/		
	return len;
}
	
// Definition of the attributes (files created in /sys/class/ledpanel) 
// used by the ledpanel driver class and related callback function to be 
// called when someone in user space write on this files
static struct class_attribute ledpanel_class_attrs[] = {
   __ATTR(rgb_buffer,   0200, NULL, ledpanel_rgb_buffer),
   __ATTR_NULL,
};

// Name of directory created in /sys/class for this driver
static struct class ledpanel_class = {
  .name =        "ledpanel",
  .owner =       THIS_MODULE,
  .class_attrs = ledpanel_class_attrs,
};

// Set the initial state of GPIO lines
static int ledpanel_gpio_init(void) {
	int rtc,i;

	for (i=0;i<sizeof(ledpanel_gpio);i++) {
		rtc=gpio_request(ledpanel_gpio[i],ledpanel_label[i]);
		if (rtc!=0) return -1;
    }
    
	for (i=0;i<sizeof(ledpanel_gpio);i++) {
		rtc=gpio_direction_output(ledpanel_gpio[i],0);
		if (rtc!=0) return -1;
    }

    gpio_set_value(ledpanel_gpio[LEDPANEL_A],0);
    gpio_set_value(ledpanel_gpio[LEDPANEL_B],0);
    gpio_set_value(ledpanel_gpio[LEDPANEL_C],0);
    gpio_set_value(ledpanel_gpio[LEDPANEL_D],0);
    gpio_set_value(ledpanel_gpio[LEDPANEL_CLK],0);
    gpio_set_value(ledpanel_gpio[LEDPANEL_STB],0);
    gpio_set_value(ledpanel_gpio[LEDPANEL_OE],1);
    gpio_set_value(ledpanel_gpio[LEDPANEL_TP],0);
	return 0;
}

// Send a new row the panel
// Callback function called by the hrtimer
enum hrtimer_restart ledpanel_hrtimer_callback(struct hrtimer *timer){
	int col;
	ktime_t cb_timer_now = ktime_set(0,0);
	ktime_t cb_timer = ktime_set(0, 25000); // 
	
	TP_HI;
	
	if (newframe==1) {
		newframe=0;
		ledpanel_row=0;
		pbuffer_top=TOP_BYTE_ARRAY;
		pbuffer_bottom=BOTTOM_BYTE_ARRAY;
	}
   
	for (col=0;col<32;col++) {

		//RED 0
		if (rgb_buffer_copy[pbuffer_top]==0) {
			*((unsigned long *)PB_CODR) = R0_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_top]<=rgb_buffer[pbuffer_top]) {
				*((unsigned long *)PB_SODR) = R0_MASK;
			}
		}
		if (rgb_buffer[pbuffer_top]>0) {
			rgb_buffer_copy[pbuffer_top]--;
			rgb_buffer_copy[pbuffer_top]&=COLOR_MASK;
		}
		pbuffer_top++;
		
		//GREEN 0
		if (rgb_buffer_copy[pbuffer_top]==0) {
			*((unsigned long *)PB_CODR) = G0_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_top]<=rgb_buffer[pbuffer_top]) {
				*((unsigned long *)PB_SODR) = G0_MASK;
			}
		}
		if (rgb_buffer[pbuffer_top]>0) {
			rgb_buffer_copy[pbuffer_top]--;
			rgb_buffer_copy[pbuffer_top]&=COLOR_MASK;
		}
		pbuffer_top++;

		//BLUE 0
		if (rgb_buffer_copy[pbuffer_top]==0) {
			*((unsigned long *)PB_CODR) = B0_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_top]<=rgb_buffer[pbuffer_top]) {
				*((unsigned long *)PB_SODR) = B0_MASK;
			}
		}
		if (rgb_buffer[pbuffer_top]>0) {
			rgb_buffer_copy[pbuffer_top]--;
			rgb_buffer_copy[pbuffer_top]&=COLOR_MASK;
		}
		pbuffer_top++;

		//RED 1
		if (rgb_buffer_copy[pbuffer_bottom]==0) {
			*((unsigned long *)PB_CODR) = R1_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_bottom]<=rgb_buffer[pbuffer_bottom]) {
				*((unsigned long *)PB_SODR) = R1_MASK;
			}
		}
		if (rgb_buffer[pbuffer_bottom]>0) {
			rgb_buffer_copy[pbuffer_bottom]--;
			rgb_buffer_copy[pbuffer_bottom]&=COLOR_MASK;
		}
		pbuffer_bottom++;

		//GREEN 1
		if (rgb_buffer_copy[pbuffer_bottom]==0) {
			*((unsigned long *)PB_CODR) = G1_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_bottom]<=rgb_buffer[pbuffer_bottom]) {
				*((unsigned long *)PB_SODR) = G1_MASK;
			}
		}
		if (rgb_buffer[pbuffer_bottom]>0) {
			rgb_buffer_copy[pbuffer_bottom]--;
			rgb_buffer_copy[pbuffer_bottom]&=COLOR_MASK;
		}
		pbuffer_bottom++;

		//BLUE 1
		if (rgb_buffer_copy[pbuffer_bottom]==0) {
			*((unsigned long *)PB_CODR) = B1_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_bottom]<=rgb_buffer[pbuffer_bottom]) {
				*((unsigned long *)PB_SODR) = B1_MASK;
			}
		}
		if (rgb_buffer[pbuffer_bottom]>0) {
			rgb_buffer_copy[pbuffer_bottom]--;
			rgb_buffer_copy[pbuffer_bottom]&=COLOR_MASK;
		}
		pbuffer_bottom++;


		CLK_HI;
		CLK_LO;
	}		

	// Disable the OE
    gpio_set_value(ledpanel_gpio[LEDPANEL_OE],1);

	// Change A,B,C,D
	*((unsigned long *)PB_CODR) = (0xF << A_PPOS);
	*((unsigned long *)PB_SODR) = (ledpanel_row << A_PPOS);

    gpio_set_value(ledpanel_gpio[LEDPANEL_STB],1);
	gpio_set_value(ledpanel_gpio[LEDPANEL_STB],0);
	gpio_set_value(ledpanel_gpio[LEDPANEL_OE],0);
	
	ledpanel_row++;
	if (ledpanel_row>=16) {
		ledpanel_row=0;
		pbuffer_top=TOP_BYTE_ARRAY;
		pbuffer_bottom=BOTTOM_BYTE_ARRAY;
	}

#if 1
	hrtimer_start(&hr_timer, ktime_set(0,25000), HRTIMER_MODE_REL);
	TP_LO;
 	return HRTIMER_NORESTART;
#else
	hrtimer_forward(&hr_timer, hrtimer_cb_get_time( &hr_timer), ktime_set(0,25000));
	TP_LO;
	return HRTIMER_RESTART;
#endif
}

static int ledpanel_init(void)
{
	struct timespec tp;
	
    printk(KERN_INFO "Ledpanel (pwm) driver v1.00 - initializing.\n");

	if (class_register(&ledpanel_class)<0) goto fail;
    
	hrtimer_get_res(CLOCK_MONOTONIC, &tp);
	printk(KERN_INFO "Clock resolution: %ldns\n", tp.tv_nsec);
      
    if (ledpanel_gpio_init()!=0) {
		printk(KERN_INFO "Error during the GPIO allocation.\n");
		class_unregister(&ledpanel_class);
		return -1;
	}	 
	
	printk(KERN_INFO "Max RGB buffer len: %d bytes\n", MAXBUFFER_PER_PANEL);
	
	
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = &ledpanel_hrtimer_callback;
	hrtimer_start(&hr_timer, ktime_set(0,0), HRTIMER_MODE_REL);
	return 0;

fail:
	return -1;

}
 
static void ledpanel_exit(void)
{
	int i;
	
	hrtimer_cancel(&hr_timer);
	gpio_set_value(ledpanel_gpio[LEDPANEL_OE],1);
 
 	for (i=0;i<sizeof(ledpanel_gpio);i++)
		gpio_free(ledpanel_gpio[i]);

	class_unregister(&ledpanel_class);
    printk(KERN_INFO "Ledpanel disabled.\n");
}
 
module_init(ledpanel_init);
module_exit(ledpanel_exit);
