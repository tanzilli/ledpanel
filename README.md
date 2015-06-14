#Simple Linux driver for a 32x32 RGB led panel

Bit banging driver to manage a RGB led panel using the  
[Arieta G25 Linux Embedded SoM](http://www.acmesystems.it/arietta)

* [Video: Sliding clock](http://www.youtube.com/embed/Qszwey7jYl4)

##How to install

Move inside the __linux/drivers__ Linux source directory 
and clone the ledpanel git repository:

	$ git clone git://github.com/tanzilli/ledpanel.git

add this line in __linux/driver/Makefile__:

	obj-$(CONFIG_LEDPANEL)   += ledpanel/

add this line in__linux/driver/Kconfig__:

	source "drivers/ledpanel/Kconfig"

Run __make menuconfig__ and enable the ledpanel driver:

	Device Drivers  --->
		<M> RGB led panel bit banging driver (NEW) 

and the High Resolution Timer Support:

	General setup  --->
		Timers subsystem  --->
			[*] High Resolution Timer Support  

##How to use it

Create a 32x32*3 image byte array (24 bit for any pixel) and save in
on __/sys/class/ledpanel/rgb_buffer__.

More examples are available on:

* [Led Panel home page](http://www.acmesystems.it/ledpanel)



#Where to buy:

* [Acme Systems eShop](http://www.acmesystems.it/catalog_arietta)

(c) 2015 - Sergio Tanzilli - sergio@tanzilli.com
