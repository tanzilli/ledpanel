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
 
// Arietta G25 GPIO lines used
// http://www.acmesystems.it/pinout_arietta
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
	"OE", 
	"CLK",
	"STB"
}; 

// This function is called when you write something on /sys/class/ledpanel/rgb_buffer
// passing in *buf the incoming content

// rgb_buffer is the content to show on the panel(s) in rgb 8 bit format
static ssize_t ledpanel_rgb_buffer(struct class *class, struct class_attribute *attr, const char *buf, size_t len) {
	mutex_lock(&sysfs_lock);
	if ((len<=MAXBUFFER_PER_PANEL)) {
		memset(rgb_buffer,MAXBUFFER_PER_PANEL,0);
		memcpy(rgb_buffer,buf,len);
	} else {
		memcpy(rgb_buffer,buf,MAXBUFFER_PER_PANEL);
	}		
	mutex_unlock(&sysfs_lock);
	//printk(KERN_INFO "Buffer len %ld bytes\n", len);
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

// Send on A,B,C,D lines the row address (0 to 15)
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

    gpio_set_value(ledpanel_gpio[LEDPANEL_OE],1);
    gpio_set_value(ledpanel_gpio[LEDPANEL_CLK],0);
    gpio_set_value(ledpanel_gpio[LEDPANEL_STB],0);
	return 0;
}

// Send a new row the panel
// Callback function called by the hrtimer
enum hrtimer_restart ledpanel_hrtimer_callback(struct hrtimer *timer){
	int col;

	gpio_set_value(ledpanel_gpio[LEDPANEL_OE],1);	
	ledpanel_set_ABCD(ledpanel_row);
	for (col=0;col<32;col++) {
		gpio_set_value(ledpanel_gpio[LEDPANEL_R0],rgb_buffer[pbuffer_top+0]);
		gpio_set_value(ledpanel_gpio[LEDPANEL_G0],rgb_buffer[pbuffer_top+1]);
		gpio_set_value(ledpanel_gpio[LEDPANEL_B0],rgb_buffer[pbuffer_top+2]);
		gpio_set_value(ledpanel_gpio[LEDPANEL_R1],rgb_buffer[pbuffer_bottom+0]);
		gpio_set_value(ledpanel_gpio[LEDPANEL_G1],rgb_buffer[pbuffer_bottom+1]);
		gpio_set_value(ledpanel_gpio[LEDPANEL_B1],rgb_buffer[pbuffer_bottom+2]);
		
		gpio_set_value(ledpanel_gpio[LEDPANEL_CLK],1);
		gpio_set_value(ledpanel_gpio[LEDPANEL_CLK],0);

		pbuffer_top+=3;
		pbuffer_bottom+=3;
	}		
	gpio_set_value(ledpanel_gpio[LEDPANEL_STB],1);
	gpio_set_value(ledpanel_gpio[LEDPANEL_STB],0);
	gpio_set_value(ledpanel_gpio[LEDPANEL_OE],0);	
	
	if ((++ledpanel_row)==16) {
		ledpanel_row=0;
		pbuffer_top=0;
		pbuffer_bottom=1536;
	}

	hrtimer_start(&hr_timer, ktime_set(0,0), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

static int ledpanel_init(void)
{
	struct timespec tp;
	
    printk(KERN_INFO "Ledpanel driver v1.01 initializing.\n");

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
	hrtimer_start(&hr_timer, ktime_set(0,200000), HRTIMER_MODE_REL);
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
