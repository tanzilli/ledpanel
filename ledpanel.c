/*
 * linux/driver/ledpanel/ledpanel.c
 *
 * Bit banging Linux driver for RGB 32x32 led panel
 * Optimized version with brightness control
 * Version working with Kernel >= 4.1
 *
 * Copyright (C) 2015 Sergio Tanzilli, All Rights Reserved.
 *
 * sergio@tanzilli.com
 * http://www.acmesystems.it/ledpanel
 *
 * Many thanks to
 * Alexandre Belloni <alexandre.belloni@free-electrons.com>
 * for his contribute
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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
#include <linux/random.h>
#include <linux/string.h>

#include <asm/io.h>

#define MAXBUFFER_PER_PANEL 32*32*3

MODULE_LICENSE("Dual BSD/GPL");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sergio Tanzilli");
MODULE_DESCRIPTION("Bit banging driver for RGB 32x32 LED PANEL");

#define BRIGHTNESS_LEVEL    16

static struct hrtimer hr_timer;
static int ledpanel_row=0;

static DEFINE_MUTEX(sysfs_lock);
static unsigned char rgb_buffer[MAXBUFFER_PER_PANEL];
static unsigned char pwm_buffer[32*16*BRIGHTNESS_LEVEL];
static int pwm_buffer_index=0;

// 0xFFFFF400 (PIOA)
#define PIO_PER		0x00	/* Enable Register */
#define PIO_OER		0x10	/* Output Enable Register */
#define PIO_SODR	0x30	/* Set Output Data Register */
#define PIO_CODR	0x34	/* Clear Output Data Register */

// Used to get access to PIO register
static void __iomem *pioa;

// RGB lines for the top side
#define R0_MASK 1 << 21
#define G0_MASK 1 << 22
#define B0_MASK 1 << 23

// RGB lines for the bottom side
#define R1_MASK 1 << 24
#define G1_MASK 1 << 25
#define B1_MASK 1 << 26

// Row address
#define A_MASK  1 << 5
#define B_MASK  1 << 6
#define C_MASK  1 << 7
#define D_MASK  1 << 8

// Control signal
#define CLK_MASK	1 << 27 // Clock (low to hi)
#define STB_MASK	1 << 28 // Strobe (low to hi)
#define OE_MASK		1 << 29 // Output enable (active low)

#define ALL_MASK		(R0_MASK | G0_MASK | B0_MASK | R1_MASK | G1_MASK | B1_MASK | A_MASK | B_MASK | C_MASK | D_MASK | CLK_MASK | STB_MASK | OE_MASK)
#define ALLWHITE_MASK	(R0_MASK | G0_MASK | B0_MASK | R1_MASK | G1_MASK | B1_MASK)
#define ALLBLACK_MASK	~(R0_MASK | G0_MASK | B0_MASK | R1_MASK | G1_MASK | B1_MASK)

// This function is called when you write the rgb contents on /sys/class/ledpanel/rgb_buffer
// To improve the speed the rgb buffer is converted in a stream of data ready to send
// to the LCD pannel to implements the brightness control

static ssize_t ledpanel_rgb_buffer(struct class *class, struct class_attribute *attr, const char *buf, size_t len) {
	int i;
	int index_pwm, pwm_panel;

	//mutex_lock(&sysfs_lock);

	//rgb buffer clean up
	memset(rgb_buffer,0,MAXBUFFER_PER_PANEL);

	//pwm buffer clean up
	memset(pwm_buffer,0,32*16*BRIGHTNESS_LEVEL);

	if ((len<=MAXBUFFER_PER_PANEL)) {
		memcpy(rgb_buffer,buf,len);
	} else {
		memcpy(rgb_buffer,buf,MAXBUFFER_PER_PANEL);
	}
	//printk(KERN_INFO "Buffer len %d bytes\n", len);

	// Convert the received RGB buffer in PWM buffer

	// RGB buffer is the image to show in RGB 8+8+8 format

	// | 1 byte red | 1 byte green | 1 byte blue | X 32 columns x 32 rows

	// PWM buffer contain the "scene" to send to the led matrix.
	// Each byte contains | -- -- B1 G1 R1 B0 G0 R0 |
	// Each scene is 512 bytes lenght
	// (32x32 (leds) x3 (color) / 6 (line for 2 leds) = 512)

	// To have 16 brightness level this driver sents to the panel
	// a stream of 16 scene so 512x16 = 8.192 byte

	index_pwm=0;
	for (pwm_panel=0;pwm_panel<BRIGHTNESS_LEVEL;pwm_panel++) {
		for (i=0;i<1536;i+=3) {
			// R0
			if (rgb_buffer[i+0]>0) {
				pwm_buffer[index_pwm]|=0x01;
				rgb_buffer[i+0]--;
			}
			// G0
			if (rgb_buffer[i+1]>0) {
				pwm_buffer[index_pwm]|=0x02;
				rgb_buffer[i+1]--;
			}
			// B0
			if (rgb_buffer[i+2]>0) {
				pwm_buffer[index_pwm]|=0x04;
				rgb_buffer[i+2]--;
			}

			// R1
			if (rgb_buffer[i+0+1536]>0) {
				pwm_buffer[index_pwm]|=0x08;
				rgb_buffer[i+0+1536]--;
			}
			// G1
			if (rgb_buffer[i+1+1536]>0) {
				pwm_buffer[index_pwm]|=0x10;
				rgb_buffer[i+1+1536]--;
			}
			// B1
			if (rgb_buffer[i+2+1536]>0) {
				pwm_buffer[index_pwm]|=0x20;
				rgb_buffer[i+2+1536]--;
			}
			index_pwm++;
		}
	}
	//mutex_unlock(&sysfs_lock);
	ledpanel_row=0;
	pwm_buffer_index=0;
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

// Send a new row the panel
// Callback function called by the hrtimer each 67uS
enum hrtimer_restart ledpanel_hrtimer_callback(struct hrtimer *timer){
	int col;
	for (col=0;col<32;col++) {
		writel_relaxed(ALLWHITE_MASK, pioa + PIO_CODR);

		//Test all led white
		//writel_relaxed(ALLWHITE_MASK, pioa + PIO_SODR);

		writel_relaxed(pwm_buffer[pwm_buffer_index]<<21, pioa + PIO_SODR);

		pwm_buffer_index++;

		// Send a clock pulse
		writel_relaxed(CLK_MASK, pioa + PIO_SODR);
		writel_relaxed(CLK_MASK, pioa + PIO_CODR);
 	}

 	//Disable OE
	writel_relaxed(OE_MASK, pioa + PIO_SODR);

	// Increment address on A,B,C,D
	writel_relaxed(0xF << 5, pioa + PIO_CODR);
	writel_relaxed(ledpanel_row << 5, pioa + PIO_SODR);

	// Strobe pulse
	writel_relaxed(STB_MASK, pioa + PIO_SODR);
	writel_relaxed(STB_MASK, pioa + PIO_CODR);

	ledpanel_row++;
	if (ledpanel_row>=16) ledpanel_row=0;

	// Impulso da 456nS
	// gpio_set_value(ledpanel_gpio[LEDPANEL_R0],1);
	// gpio_set_value(ledpanel_gpio[LEDPANEL_R0],0);

	// Impulso da 38nS
	// writel_relaxed(R0_MASK, pioa + PIO_CODR);
	// writel_relaxed(R0_MASK, pioa + PIO_SODR);

	if (pwm_buffer_index>=(512*BRIGHTNESS_LEVEL))
		pwm_buffer_index=0;

	// Enable OE
	writel_relaxed(OE_MASK, pioa + PIO_CODR);

	hrtimer_start(&hr_timer, ktime_set(0,12000), HRTIMER_MODE_REL);
 	return HRTIMER_NORESTART;
}

static int ledpanel_init(void)
{
	struct timespec tp;

    printk(KERN_INFO "Ledpanel driver 1.00 - initializing.\n");

	if (class_register(&ledpanel_class)<0) goto fail;

	hrtimer_get_res(CLOCK_MONOTONIC, &tp);
	printk(KERN_INFO "Clock resolution: %ldns\n", tp.tv_nsec);

	pioa = ioremap(0xFFFFF400, 0x200);
	writel_relaxed(ALL_MASK, pioa + PIO_PER);
	writel_relaxed(ALL_MASK, pioa + PIO_OER);
	writel_relaxed(ALL_MASK, pioa + PIO_CODR);
	writel_relaxed(OE_MASK, pioa + PIO_SODR);

	printk(KERN_INFO "Brightness levels: %d\n", BRIGHTNESS_LEVEL);
	printk(KERN_INFO "RGB buffer lenght: %d bytes\n", MAXBUFFER_PER_PANEL);
	printk(KERN_INFO "PWM buffer lenght: %d bytes\n", 32*16*BRIGHTNESS_LEVEL);

	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function = &ledpanel_hrtimer_callback;
	hrtimer_start(&hr_timer, ktime_set(0,0), HRTIMER_MODE_REL);
	return 0;

fail:
	printk(KERN_INFO "Ledpanel error\n");
	return -1;

}

static void ledpanel_exit(void)
{
	hrtimer_cancel(&hr_timer);
	writel_relaxed(OE_MASK, pioa + PIO_SODR);
	iounmap(pioa);
	class_unregister(&ledpanel_class);
    printk(KERN_INFO "Ledpanel disabled.\n");
}

module_init(ledpanel_init);
module_exit(ledpanel_exit);
