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
static int ledpanel_row=0;
 
// Arietta G25 GPIO lines used
// http://www.acmesystems.it/pinout_arietta
// http://www.acmesystems.it/P6LED3232
 
// GPIO lines used 
static char ledpanel_gpio[] = {
	22, // R0
	21, // G0
	31, // B0 
	
	30, // R1
	1,  // G1 
	7,  // B1
	
	5,  // A
	91, // B
	95, // C
	43, // D
	
	46, // OE 
	44, // CLK
	45  // STB
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
	"OE", 
	"CLK",
	"STB"
}; 

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
#define LEDPANEL_OE		10 
#define LEDPANEL_CLK	11 
#define LEDPANEL_STB	12 

static ssize_t ledpanel_buffer32x32(struct class *class, struct class_attribute *attr, const char *buf, size_t len){
	printk(KERN_INFO "Buffer lenght %d.\n",len);
	return len;
}

/* Sysfs definitions for ledpanel class */
static struct class_attribute ledpanel_class_attrs[] = {
   __ATTR(buffer32x32,   0200, NULL, ledpanel_buffer32x32),
   __ATTR_NULL,
};

static struct class ledpanel_class = {
  .name =        "ledpanel",
  .owner =       THIS_MODULE,
  .class_attrs = ledpanel_class_attrs,
};

// Set the row address (0-15) on A,B,C,D line
static void ledpanel_set_ABCD(unsigned char address) 
{
	gpio_set_value(ledpanel_gpio[LEDPANEL_A],0);
	gpio_set_value(ledpanel_gpio[LEDPANEL_B],0);
	gpio_set_value(ledpanel_gpio[LEDPANEL_C],0);
	gpio_set_value(ledpanel_gpio[LEDPANEL_D],0);

	if (address & 1) 
		gpio_set_value(ledpanel_gpio[LEDPANEL_A],1);
	if (address & 2) 
		gpio_set_value(ledpanel_gpio[LEDPANEL_B],1);
	if (address & 4) 
		gpio_set_value(ledpanel_gpio[LEDPANEL_C],1);
	if (address & 8) 
		gpio_set_value(ledpanel_gpio[LEDPANEL_D],1);
} 
 
static void ledpanel_pattern(unsigned char red,unsigned char green,unsigned char blue) 
{
	int col;
	
	gpio_set_value(ledpanel_gpio[LEDPANEL_OE],1);	
	for (col=0;col<16;col++) {
		gpio_set_value(ledpanel_gpio[LEDPANEL_R0],red);
		gpio_set_value(ledpanel_gpio[LEDPANEL_G0],green);
		gpio_set_value(ledpanel_gpio[LEDPANEL_B0],blue);
		gpio_set_value(ledpanel_gpio[LEDPANEL_R1],red);
		gpio_set_value(ledpanel_gpio[LEDPANEL_G1],green);
		gpio_set_value(ledpanel_gpio[LEDPANEL_B1],blue);
		
		gpio_set_value(ledpanel_gpio[LEDPANEL_CLK],1);
		gpio_set_value(ledpanel_gpio[LEDPANEL_CLK],0);
		
		gpio_set_value(ledpanel_gpio[LEDPANEL_R0],red);
		gpio_set_value(ledpanel_gpio[LEDPANEL_G0],green);
		gpio_set_value(ledpanel_gpio[LEDPANEL_B0],blue);
		gpio_set_value(ledpanel_gpio[LEDPANEL_R1],red);
		gpio_set_value(ledpanel_gpio[LEDPANEL_G1],green);
		gpio_set_value(ledpanel_gpio[LEDPANEL_B1],blue);
		
		gpio_set_value(ledpanel_gpio[LEDPANEL_CLK],1);
		gpio_set_value(ledpanel_gpio[LEDPANEL_CLK],0);

		ledpanel_set_ABCD(ledpanel_row);
		
		gpio_set_value(ledpanel_gpio[LEDPANEL_STB],1);
		gpio_set_value(ledpanel_gpio[LEDPANEL_STB],0);
		gpio_set_value(ledpanel_gpio[LEDPANEL_OE],0);	
	}		
	
	ledpanel_row++;
	if (ledpanel_row>=16) {
		ledpanel_row=0;
	}
} 


/* Set the initial state of GPIO lines */
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

    gpio_set_value(ledpanel_gpio[LEDPANEL_OE],1);
    gpio_set_value(ledpanel_gpio[LEDPANEL_CLK],0);
    gpio_set_value(ledpanel_gpio[LEDPANEL_STB],0);
	return 0;
}

// Callback function called by the hrtimer
enum hrtimer_restart ledpanel_hrtimer_callback(struct hrtimer *timer){
	hrtimer_start(&hr_timer, ktime_set(0,0), HRTIMER_MODE_REL);
	ledpanel_pattern(1,1,1);
	return HRTIMER_NORESTART;
}

static int ledpanel_init(void)
{
	struct timespec tp;
	
    printk(KERN_INFO "Ledpanel driver v0.08 initializing.\n");

	if (class_register(&ledpanel_class)<0) goto fail;

    
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
