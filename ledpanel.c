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

MODULE_LICENSE("Dual BSD/GPL");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergio Tanzilli");
MODULE_DESCRIPTION("Driver for 32x32 RGB LCD PANELS");
 
static struct hrtimer hr_timer; 
int ledpanel_row=0;
 
// Arietta G25 GPIO lines used
// http://www.acmesystems.it/pinout_arietta
// http://www.acmesystems.it/P6LED3232
 
#define LEDPANEL_R0 22 
#define LEDPANEL_G0 21 
#define LEDPANEL_B0 31 

#define LEDPANEL_R1 30 
#define LEDPANEL_G1 1 
#define LEDPANEL_B1 7 

#define LEDPANEL_A 5 
#define LEDPANEL_B 91 
#define LEDPANEL_C 95 
#define LEDPANEL_D 43 

#define LEDPANEL_OE 46 
#define LEDPANEL_CLK 44 
#define LEDPANEL_STB 45 

// Set the row address (0-15) on A,B,C,D line
static void ledpanel_set_ABCD(unsigned char address) 
{
	gpio_set_value(LEDPANEL_A,0);
	gpio_set_value(LEDPANEL_B,0);
	gpio_set_value(LEDPANEL_C,0);
	gpio_set_value(LEDPANEL_D,0);

	if (address & 1) 
		gpio_set_value(LEDPANEL_A,1);
	if (address & 2) 
		gpio_set_value(LEDPANEL_B,1);
	if (address & 4) 
		gpio_set_value(LEDPANEL_C,1);
	if (address & 8) 
		gpio_set_value(LEDPANEL_D,1);
} 
 
static void ledpanel_pattern(unsigned char red,unsigned char green,unsigned char blue) 
{
	int col;
	
	gpio_set_value(LEDPANEL_OE,1);	
	for (col=0;col<16;col++) {
		gpio_set_value(LEDPANEL_R0,red);
		gpio_set_value(LEDPANEL_G0,green);
		gpio_set_value(LEDPANEL_B0,blue);
		gpio_set_value(LEDPANEL_R1,red);
		gpio_set_value(LEDPANEL_G1,green);
		gpio_set_value(LEDPANEL_B1,blue);
		
		gpio_set_value(LEDPANEL_CLK,1);
		gpio_set_value(LEDPANEL_CLK,0);
		
		gpio_set_value(LEDPANEL_R0,red);
		gpio_set_value(LEDPANEL_G0,green);
		gpio_set_value(LEDPANEL_B0,blue);
		gpio_set_value(LEDPANEL_R1,red);
		gpio_set_value(LEDPANEL_G1,green);
		gpio_set_value(LEDPANEL_B1,blue);
		
		gpio_set_value(LEDPANEL_CLK,1);
		gpio_set_value(LEDPANEL_CLK,0);

		ledpanel_set_ABCD(ledpanel_row);
		
		gpio_set_value(LEDPANEL_STB,1);
		gpio_set_value(LEDPANEL_STB,0);
		gpio_set_value(LEDPANEL_OE,0);	
	}		
	
	ledpanel_row++;
	if (ledpanel_row>=16) {
		ledpanel_row=0;
	}
} 


/* Set the initial state of GPIO lines */
static int ledpanel_gpio_init(void) {
	int rtc;

    rtc=gpio_request(LEDPANEL_OE,"oe");
    if (rtc!=0) return -1;
    rtc=gpio_direction_output(LEDPANEL_OE,1);
    if (rtc!=0) return -1;

    rtc=gpio_request(LEDPANEL_CLK,"clk");
    if (rtc!=0) return -1;
    rtc=gpio_direction_output(LEDPANEL_CLK,0);
    if (rtc!=0) return -1;
    
    rtc=gpio_request(LEDPANEL_STB,"stb");
    if (rtc!=0) return -1;
    rtc=gpio_direction_output(LEDPANEL_STB,0);
    if (rtc!=0) return -1;

    rtc=gpio_request(LEDPANEL_A,"a");
    if (rtc!=0) return -1;
    rtc=gpio_direction_output(LEDPANEL_A,0);
    if (rtc!=0) return -1;
    
    rtc=gpio_request(LEDPANEL_B,"b");
    if (rtc!=0) return -1;
    rtc=gpio_direction_output(LEDPANEL_B,0);
    if (rtc!=0) return -1;
    
    rtc=gpio_request(LEDPANEL_C,"c");
    if (rtc!=0) return -1;
    rtc=gpio_direction_output(LEDPANEL_C,0);
    if (rtc!=0) return -1;
    
    rtc=gpio_request(LEDPANEL_D,"D");
    if (rtc!=0) return -1;    
    rtc=gpio_direction_output(LEDPANEL_D,0);
    if (rtc!=0) return -1;

    rtc=gpio_request(LEDPANEL_R0,"r0");
    if (rtc!=0) return -1;    
    rtc=gpio_direction_output(LEDPANEL_R0,0);
    if (rtc!=0) return -1;

    rtc=gpio_request(LEDPANEL_G0,"g0");
    if (rtc!=0) return -1;
    rtc=gpio_direction_output(LEDPANEL_G0,0);
    if (rtc!=0) return -1;
    
    rtc=gpio_request(LEDPANEL_B0,"b0");
    if (rtc!=0) return -1;
    rtc=gpio_direction_output(LEDPANEL_B0,0);
    if (rtc!=0) return -1;
    
    rtc=gpio_request(LEDPANEL_R1,"r1");
    if (rtc!=0) return -1;    
    rtc=gpio_direction_output(LEDPANEL_R1,0);
    if (rtc!=0) return -1;
    
    rtc=gpio_request(LEDPANEL_G1,"g1");
    if (rtc!=0) return -1;    
    rtc=gpio_direction_output(LEDPANEL_G1,0);
    if (rtc!=0) return -1;
    
    rtc=gpio_request(LEDPANEL_B1,"b1");
    if (rtc!=0) return -1;    
    rtc=gpio_direction_output(LEDPANEL_B1,0);
    if (rtc!=0) return -1;
	return 0;
}

enum hrtimer_restart ledpanel_hrtimer_callback(struct hrtimer *timer){
	hrtimer_start(&hr_timer, ktime_set(0,0), HRTIMER_MODE_REL);
	ledpanel_pattern(1,1,1);
	return HRTIMER_NORESTART;
}

static int ledpanel_init(void)
{
	struct timespec tp;
	
    printk(KERN_INFO "Ledpanel driver v0.06 initializing.\n");
    
	hrtimer_get_res(CLOCK_MONOTONIC, &tp);
	printk(KERN_INFO "Clock resolution is %ldns\n", tp.tv_nsec);
      
    if (ledpanel_gpio_init()!=0) {
		printk(KERN_INFO "Error during the GPIO allocation.\n");
		return -1;
	}	 
	
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = &ledpanel_hrtimer_callback;
	hrtimer_start(&hr_timer, ktime_set(0,0), HRTIMER_MODE_REL);
	
	printk(KERN_INFO "Ledpanel initialized.\n");
	return 0;

}
 
static void ledpanel_exit(void)
{
	hrtimer_cancel(&hr_timer);
	gpio_set_value(LEDPANEL_OE,1);
    printk(KERN_INFO "Ledpanel disabled.\n");
}
 
module_init(ledpanel_init);
module_exit(ledpanel_exit);
