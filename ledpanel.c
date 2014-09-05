#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>
 
MODULE_LICENSE("Dual BSD/GPL");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergio Tanzilli");
MODULE_DESCRIPTION("Driver for 32x32 RGB LCD PANELS");
 
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

static void SetABCD(unsigned char address) 
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

static void LatchesReset(void) 
{
	int i;
		
	gpio_set_value(LEDPANEL_R0,0);
	gpio_set_value(LEDPANEL_G0,0);
	gpio_set_value(LEDPANEL_B0,0);
	gpio_set_value(LEDPANEL_R1,0);
	gpio_set_value(LEDPANEL_G1,0);
	gpio_set_value(LEDPANEL_B1,0);
	
	for (i=0;i<192;i++) {
		gpio_set_value(LEDPANEL_CLK,1);
		gpio_set_value(LEDPANEL_CLK,0);
	}
	
	gpio_set_value(LEDPANEL_STB,1);
	gpio_set_value(LEDPANEL_STB,0);
} 
 
static void LedPattern(void) 
{
	int r,z,p;
	
	for (z=0;z<1000;z++) {
		for (r=0;r<16;r++) {
			for (p=0;p<16;p++) {
				gpio_set_value(LEDPANEL_R0,0);
				gpio_set_value(LEDPANEL_G0,1);
				gpio_set_value(LEDPANEL_B0,0);
				gpio_set_value(LEDPANEL_R1,0);
				gpio_set_value(LEDPANEL_G1,1);
				gpio_set_value(LEDPANEL_B1,0);
				
				gpio_set_value(LEDPANEL_CLK,1);
				gpio_set_value(LEDPANEL_CLK,0);
				
				gpio_set_value(LEDPANEL_R0,1);
				gpio_set_value(LEDPANEL_G0,1);
				gpio_set_value(LEDPANEL_B0,1);
				gpio_set_value(LEDPANEL_R1,1);
				gpio_set_value(LEDPANEL_G1,1);
				gpio_set_value(LEDPANEL_B1,1);
				
				gpio_set_value(LEDPANEL_CLK,1);
				gpio_set_value(LEDPANEL_CLK,0);
			}		
			
			gpio_set_value(LEDPANEL_OE,1);	
			SetABCD(r);
			gpio_set_value(LEDPANEL_STB,1);
			gpio_set_value(LEDPANEL_STB,0);
			gpio_set_value(LEDPANEL_OE,0);	

		}
	}		
	gpio_set_value(LEDPANEL_OE,1);

} 

static int hello_init(void)
{
	int rtc;
    
    printk(KERN_INFO "RGBLedPanel driver v0.05 initializing.\n");
    
    rtc=gpio_direction_output(LEDPANEL_OE,1);
    printk(KERN_ALERT "OE=%d\n",rtc);
 
    rtc=gpio_direction_output(LEDPANEL_CLK,0);
    printk(KERN_ALERT "CLK=%d\n",rtc);

    rtc=gpio_direction_output(LEDPANEL_STB,0);
    printk(KERN_ALERT "STB=%d\n",rtc);
 
    rtc=gpio_direction_output(LEDPANEL_A,0);
    printk(KERN_ALERT "A=%d\n",rtc);

    rtc=gpio_direction_output(LEDPANEL_B,0);
    printk(KERN_ALERT "B=%d\n",rtc);

    rtc=gpio_direction_output(LEDPANEL_C,0);
    printk(KERN_ALERT "C=%d\n",rtc);
    
    rtc=gpio_direction_output(LEDPANEL_D,0);
    printk(KERN_ALERT "D=%d\n",rtc);
 
    rtc=gpio_direction_output(LEDPANEL_R0,0);
    printk(KERN_ALERT "R0=%d\n",rtc);
    
    rtc=gpio_direction_output(LEDPANEL_G0,0);
    printk(KERN_ALERT "G0=%d\n",rtc);

    rtc=gpio_direction_output(LEDPANEL_B0,0);
    printk(KERN_ALERT "B0=%d\n",rtc);

    rtc=gpio_direction_output(LEDPANEL_R1,0);
    printk(KERN_ALERT "R1=%d\n",rtc);
    
    rtc=gpio_direction_output(LEDPANEL_G1,0);
    printk(KERN_ALERT "G1=%d\n",rtc);

    rtc=gpio_direction_output(LEDPANEL_B1,0);
    printk(KERN_ALERT "B1=%d\n",rtc);

	LatchesReset();
	LedPattern();
	
	printk(KERN_INFO "RGBLedPanel initialized.\n");


	return 0;
}
 
static void hello_exit(void)
{
	gpio_set_value(LEDPANEL_OE,1);
    printk(KERN_INFO "RGBLedPanel disabled.\n");
}
 
module_init(hello_init);
module_exit(hello_exit);
