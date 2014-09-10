/*
 * ledpanel.2 
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
 
static struct hrtimer hr_timer; 
static int ledpanel_row=0;
static int pbuffer_top;
static int pbuffer_bottom=1536;

static DEFINE_MUTEX(sysfs_lock);
static unsigned char rgb_buffer[MAXBUFFER_PER_PANEL];
static unsigned char rgb_buffer_copy[MAXBUFFER_PER_PANEL];

// 3 bit per color
#define COLOR_DEPTH 8-1 // 2,4,8,16,32,64,128,256
#define COLOR_SHIFT 5 	// How many bit shift left to redure the color depth   

// 4 bit per color
//#define COLOR_DEPTH 16-1 // 2,4,8,16,32,64,128,256
//#define COLOR_SHIFT 4 	// How many bit shift right to redure the color depth   

// 0xFFFFF400 (PIOA), 0xFFFFF600 (PIOB), 0xFFFFF800 (PIOC), 0xFFFFFA00 (PIOD

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

// This function is called when you write something on /sys/class/ledpanel2/rgb_buffer
// passing in *buf the incoming content

// rgb_buffer is the content to show on the panel(s) in rgb 8 bit format
static ssize_t ledpanel_rgb_buffer(struct class *class, struct class_attribute *attr, const char *buf, size_t len) {
	int i;
	
	mutex_lock(&sysfs_lock);
	if ((len<=MAXBUFFER_PER_PANEL)) {
		memset(rgb_buffer,MAXBUFFER_PER_PANEL,0);
		memcpy(rgb_buffer,buf,len);
	} else {
		memcpy(rgb_buffer,buf,MAXBUFFER_PER_PANEL);
	}		
	
	// Color depth reduction 
	for (i=0;i<MAXBUFFER_PER_PANEL;i++) {
		rgb_buffer[i]>>=COLOR_SHIFT;
	}
	memcpy(rgb_buffer_copy,rgb_buffer,MAXBUFFER_PER_PANEL);
	mutex_unlock(&sysfs_lock);
	//printk(KERN_INFO "Buffer len %ld bytes\n", len);
	return len;
}
	
// Sysfs definitions for ledpanel class
// For any name a file in /sys/class/ledpanel2 is created
static struct class_attribute ledpanel_class_attrs[] = {
   __ATTR(rgb_buffer,   0200, NULL, ledpanel_rgb_buffer),
   __ATTR_NULL,
};

// Name of directory created in /sys/class
static struct class ledpanel_class = {
  .name =        "ledpanel2",
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
	unsigned long flags;
	
	for (col=0;col<32;col++) {
		// RED 0
		if (rgb_buffer[pbuffer_top+0]>=COLOR_DEPTH) {
			*((unsigned long *)PA_SODR) = R0_MASK;
			rgb_buffer_copy[pbuffer_top+0]&=COLOR_DEPTH;
		} else { 
			if (rgb_buffer_copy[pbuffer_top+0]==0) {
				*((unsigned long *)PA_CODR) = R0_MASK;
			} else {	
				if (rgb_buffer_copy[pbuffer_top+0]==rgb_buffer[pbuffer_top+0]) {
					*((unsigned long *)PA_SODR) = R0_MASK;
				}
			}
			rgb_buffer_copy[pbuffer_top+0]--;
			rgb_buffer_copy[pbuffer_top+0]&=COLOR_DEPTH;
		}
		
		// GREEN 0
		if (rgb_buffer[pbuffer_top+1]>=COLOR_DEPTH) {
			*((unsigned long *)PA_SODR) = G0_MASK;
			rgb_buffer_copy[pbuffer_top+1]&=COLOR_DEPTH;
		} else { 
			if (rgb_buffer_copy[pbuffer_top+1]==0) {
				*((unsigned long *)PA_CODR) = G0_MASK;
			} else {	
				if (rgb_buffer_copy[pbuffer_top+1]==rgb_buffer[pbuffer_top+1]) {
					*((unsigned long *)PA_SODR) = G0_MASK;
				}
			}
			rgb_buffer_copy[pbuffer_top+1]--;
			rgb_buffer_copy[pbuffer_top+1]&=COLOR_DEPTH;
		}

		// BLUE 0
		if (rgb_buffer[pbuffer_top+2]>=COLOR_DEPTH) {
			*((unsigned long *)PA_SODR) = B0_MASK;
			rgb_buffer_copy[pbuffer_top+2]&=COLOR_DEPTH;
		} else { 
			if (rgb_buffer_copy[pbuffer_top+2]==0) {
				*((unsigned long *)PA_CODR) = B0_MASK;
			} else {	
				if (rgb_buffer_copy[pbuffer_top+2]==rgb_buffer[pbuffer_top+2]) {
					*((unsigned long *)PA_SODR) = B0_MASK;
				}
			}
			rgb_buffer_copy[pbuffer_top+2]--;
			rgb_buffer_copy[pbuffer_top+2]&=COLOR_DEPTH;
		}

		// RED 1
		if (rgb_buffer[pbuffer_bottom+0]>=COLOR_DEPTH) {
			*((unsigned long *)PA_SODR) = R1_MASK;
			rgb_buffer_copy[pbuffer_bottom+0]&=COLOR_DEPTH;
		} else { 
			if (rgb_buffer_copy[pbuffer_bottom+0]==0) {
				*((unsigned long *)PA_CODR) = R1_MASK;
			} else {	
				if (rgb_buffer_copy[pbuffer_bottom+0]==rgb_buffer[pbuffer_bottom+0]) {
					*((unsigned long *)PA_SODR) = R1_MASK;
				}
			}
			rgb_buffer_copy[pbuffer_bottom+0]--;
			rgb_buffer_copy[pbuffer_bottom+0]&=COLOR_DEPTH;
		}

		// GREEN 1
		if (rgb_buffer[pbuffer_bottom+1]>=COLOR_DEPTH) {
			*((unsigned long *)PA_SODR) = G1_MASK;
			rgb_buffer_copy[pbuffer_bottom+1]&=COLOR_DEPTH;
		} else { 
			if (rgb_buffer_copy[pbuffer_bottom+1]==0) {
				*((unsigned long *)PA_CODR) = G1_MASK;
			} else {	
				if (rgb_buffer_copy[pbuffer_bottom+1]==rgb_buffer[pbuffer_bottom+1]) {
					*((unsigned long *)PA_SODR) = G1_MASK;
				}
			}
			rgb_buffer_copy[pbuffer_bottom+1]--;
			rgb_buffer_copy[pbuffer_bottom+1]&=COLOR_DEPTH;
		}

		// BLUE 1
		if (rgb_buffer[pbuffer_bottom+2]>=COLOR_DEPTH) {
			*((unsigned long *)PA_SODR) = B1_MASK;
			rgb_buffer_copy[pbuffer_bottom+2]&=COLOR_DEPTH;
		} else { 
			if (rgb_buffer_copy[pbuffer_bottom+2]==0) {
				*((unsigned long *)PA_CODR) = B1_MASK;
			} else {	
				if (rgb_buffer_copy[pbuffer_bottom+2]==rgb_buffer[pbuffer_bottom+2]) {
					*((unsigned long *)PA_SODR) = B1_MASK;
				}
			}
			rgb_buffer_copy[pbuffer_bottom+2]--;
			rgb_buffer_copy[pbuffer_bottom+2]&=COLOR_DEPTH;
		}

		CLK_HI;
		CLK_LO;

		pbuffer_top+=3;
		pbuffer_bottom+=3;
	}		

	*((unsigned long *)PA_CODR) = (0xF << 5);
	*((unsigned long *)PA_SODR) = (ledpanel_row << 5);
	
	local_irq_save(flags); // LDD 3 pag 274
	gpio_set_value(ledpanel_gpio[LEDPANEL_OE],1);

	STB_HI;
	STB_LO;
	
	OE_LO;
	local_irq_restore(flags);

	if ((++ledpanel_row)>=16) {
		ledpanel_row=0;
		pbuffer_top=0;
		pbuffer_bottom=1536;
	}

	hrtimer_start(&hr_timer, ktime_set(0,25000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

static int ledpanel_init(void)
{
	struct timespec tp;
	
    printk(KERN_INFO "Ledpanel2 (pwm) driver v0.03 initializing.\n");

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
    printk(KERN_INFO "Ledpanel2 disabled.\n");
}
 
module_init(ledpanel_init);
module_exit(ledpanel_exit);
