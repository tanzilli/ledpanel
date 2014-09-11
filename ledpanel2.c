/*
 * ledpanel2
 * 
 * Simple Linux driver for a RGB led panel
 * Optimized version with brightness controll
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

#define MAXBUFFER_PER_PANEL 32*32*3

MODULE_LICENSE("Dual BSD/GPL");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergio Tanzilli");
MODULE_DESCRIPTION("Driver for RGB 32x32 LED PANEL");
 
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

// 0xFFFFF400 (PIOA)

#define PA_SODR AT91_IO_P2V(0xFFFFF400+0x30) // PA SODR 
#define PA_CODR AT91_IO_P2V(0xFFFFF400+0x34) // PA CODR 

#define R0_MASK 1 << 21
#define G0_MASK 1 << 22
#define B0_MASK 1 << 23
 
#define R1_MASK 1 << 24
#define G1_MASK 1 << 25
#define B1_MASK 1 << 26
 
#define A_MASK  1 << 5
#define B_MASK  1 << 6
#define C_MASK  1 << 7
#define D_MASK  1 << 8

#define CLK_MASK	1 << 27
#define STB_MASK	1 << 28
#define OE_MASK		1 << 29

#define R0_HI	*((unsigned long *)PA_SODR) = (unsigned long)(R0_MASK);  
#define R0_LO	*((unsigned long *)PA_CODR) = (unsigned long)(R0_MASK);  
#define G0_HI	*((unsigned long *)PA_SODR) = (unsigned long)(G0_MASK);  
#define G0_LO	*((unsigned long *)PA_CODR) = (unsigned long)(G0_MASK);  
#define B0_HI	*((unsigned long *)PA_SODR) = (unsigned long)(B0_MASK);  
#define B0_LO	*((unsigned long *)PA_CODR) = (unsigned long)(B0_MASK);  

#define R1_HI	*((unsigned long *)PA_SODR) = (unsigned long)(R1_MASK);  
#define R1_LO	*((unsigned long *)PA_CODR) = (unsigned long)(R1_MASK);  
#define G1_HI	*((unsigned long *)PA_SODR) = (unsigned long)(G1_MASK);  
#define G1_LO	*((unsigned long *)PA_CODR) = (unsigned long)(G1_MASK);  
#define B1_HI	*((unsigned long *)PA_SODR) = (unsigned long)(B1_MASK);  
#define B1_LO	*((unsigned long *)PA_CODR) = (unsigned long)(B1_MASK);  

#define A_HI	*((unsigned long *)PA_SODR) = (unsigned long)(A_MASK);  
#define A_LO	*((unsigned long *)PA_CODR) = (unsigned long)(A_MASK);  
#define B_HI	*((unsigned long *)PA_SODR) = (unsigned long)(B_MASK);  
#define B_LO	*((unsigned long *)PA_CODR) = (unsigned long)(B_MASK);  
#define C_HI	*((unsigned long *)PA_SODR) = (unsigned long)(C_MASK);  
#define C_LO	*((unsigned long *)PA_CODR) = (unsigned long)(C_MASK);  
#define D_HI	*((unsigned long *)PA_SODR) = (unsigned long)(D_MASK);  
#define D_LO	*((unsigned long *)PA_CODR) = (unsigned long)(D_MASK);  

#define CLK_HI	*((unsigned long *)PA_SODR) = (unsigned long)(CLK_MASK);  
#define CLK_LO	*((unsigned long *)PA_CODR) = (unsigned long)(CLK_MASK);  
#define STB_HI	*((unsigned long *)PA_SODR) = (unsigned long)(STB_MASK);  
#define STB_LO	*((unsigned long *)PA_CODR) = (unsigned long)(STB_MASK);  
#define OE_HI	*((unsigned long *)PA_SODR) = (unsigned long)(OE_MASK);  
#define OE_LO	*((unsigned long *)PA_CODR) = (unsigned long)(OE_MASK);  

// Arietta G25 GPIO lines used
// http://www.acmesystems.it/pinout_arietta
// http://www.acmesystems.it/P6LED3232
// http://www.acmesystems.it/ledpanel
 
// GPIO lines used 
static char ledpanel_gpio[] = {
	21, // R0
	22, // G0
	23, // B0 
	
	24, // R1
	25, // G1 
	26, // B1
	
	5, // A
	6, // B
	7, // C
	8, // D
	
	27, // CLK
	28, // STB
	29, // OE 
}; 

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
	
// Sysfs definitions for ledpanel class
// For any name a file in /sys/class/ledpanel is created
static struct class_attribute ledpanel_class_attrs[] = {
   __ATTR(rgb_buffer,   0200, NULL, ledpanel_rgb_buffer),
   __ATTR_NULL,
};

// Name of directory created in /sys/class
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
	return 0;
}

// Send a new row the panel
// Callback function called by the hrtimer
enum hrtimer_restart ledpanel_hrtimer_callback(struct hrtimer *timer){
	int col;

	if (newframe==1) {
		newframe=0;
		ledpanel_row=0;
		pbuffer_top=TOP_BYTE_ARRAY;
		pbuffer_bottom=BOTTOM_BYTE_ARRAY;
	}
   
	for (col=0;col<32;col++) {

		//RED 0
		if (rgb_buffer_copy[pbuffer_top]==0) {
			*((unsigned long *)PA_CODR) = R0_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_top]<=rgb_buffer[pbuffer_top]) {
				*((unsigned long *)PA_SODR) = R0_MASK;
			}
		}
		if (rgb_buffer[pbuffer_top]>0) {
			rgb_buffer_copy[pbuffer_top]--;
			rgb_buffer_copy[pbuffer_top]&=COLOR_MASK;
		}
		pbuffer_top++;
		
		//GREEN 0
		if (rgb_buffer_copy[pbuffer_top]==0) {
			*((unsigned long *)PA_CODR) = G0_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_top]<=rgb_buffer[pbuffer_top]) {
				*((unsigned long *)PA_SODR) = G0_MASK;
			}
		}
		if (rgb_buffer[pbuffer_top]>0) {
			rgb_buffer_copy[pbuffer_top]--;
			rgb_buffer_copy[pbuffer_top]&=COLOR_MASK;
		}
		pbuffer_top++;

		//BLUE 0
		if (rgb_buffer_copy[pbuffer_top]==0) {
			*((unsigned long *)PA_CODR) = B0_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_top]<=rgb_buffer[pbuffer_top]) {
				*((unsigned long *)PA_SODR) = B0_MASK;
			}
		}
		if (rgb_buffer[pbuffer_top]>0) {
			rgb_buffer_copy[pbuffer_top]--;
			rgb_buffer_copy[pbuffer_top]&=COLOR_MASK;
		}
		pbuffer_top++;

		//RED 1
		if (rgb_buffer_copy[pbuffer_bottom]==0) {
			*((unsigned long *)PA_CODR) = R1_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_bottom]<=rgb_buffer[pbuffer_bottom]) {
				*((unsigned long *)PA_SODR) = R1_MASK;
			}
		}
		if (rgb_buffer[pbuffer_bottom]>0) {
			rgb_buffer_copy[pbuffer_bottom]--;
			rgb_buffer_copy[pbuffer_bottom]&=COLOR_MASK;
		}
		pbuffer_bottom++;

		//GREEN 1
		if (rgb_buffer_copy[pbuffer_bottom]==0) {
			*((unsigned long *)PA_CODR) = G1_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_bottom]<=rgb_buffer[pbuffer_bottom]) {
				*((unsigned long *)PA_SODR) = G1_MASK;
			}
		}
		if (rgb_buffer[pbuffer_bottom]>0) {
			rgb_buffer_copy[pbuffer_bottom]--;
			rgb_buffer_copy[pbuffer_bottom]&=COLOR_MASK;
		}
		pbuffer_bottom++;

		//BLUE 1
		if (rgb_buffer_copy[pbuffer_bottom]==0) {
			*((unsigned long *)PA_CODR) = B1_MASK;
		} else {	
			if (rgb_buffer_copy[pbuffer_bottom]<=rgb_buffer[pbuffer_bottom]) {
				*((unsigned long *)PA_SODR) = B1_MASK;
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
	*((unsigned long *)PA_CODR) = (0xF << 5);
	*((unsigned long *)PA_SODR) = (ledpanel_row << 5);

    gpio_set_value(ledpanel_gpio[LEDPANEL_STB],1);
	gpio_set_value(ledpanel_gpio[LEDPANEL_STB],0);
	gpio_set_value(ledpanel_gpio[LEDPANEL_OE],0);
	
	ledpanel_row++;
	if (ledpanel_row>=16) {
		ledpanel_row=0;
		pbuffer_top=TOP_BYTE_ARRAY;
		pbuffer_bottom=BOTTOM_BYTE_ARRAY;
	}

	hrtimer_start(&hr_timer, ktime_set(0,25000), HRTIMER_MODE_REL);
	//hrtimer_start(&hr_timer, ktime_set(0,100000000), HRTIMER_MODE_REL);
 	return HRTIMER_NORESTART;
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
