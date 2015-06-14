#Simple Linux driver for a 32x32 RGB led panel

Bit banging driver to manage a RGB led panel using the  
[Arieta G25 Linux Embedded SoM](http://www.acmesystems.it/arietta)

* [Video: Sliding clock](http://www.youtube.com/embed/Qszwey7jYl4)

##Installation

Move inside the __linux/drivers__ Linux source directory 
and clone the ledpanel git repository:

	$ git clone git://github.com/tanzilli/ledpanel.git

add the following line in __linux/driver/Makefile__:

	obj-$(CONFIG_LEDPANEL)   += ledpanel/

add the following line in__linux/driver/Kconfig__:

	source "drivers/ledpanel/Kconfig"

Run __make menuconfig__ and enable the ledpanel driver:

	Device Drivers  --->
		<*> RGB led panel bit banging driver (NEW) 

and the High Resolution Timer Support:

	General setup  --->
		Timers subsystem  --->
			[*] High Resolution Timer Support  

##Using ledpanel driver from user space

Create a 32x32*3 image byte array (24 bit for any pixel) and save in
on __/sys/class/ledpanel/rgb_buffer__ or just type:

	dd if=/dev/urandom of=/sys/class/ledpanel/rgb_buffer bs=3072 count=1

to show a random pattern.

More examples are available on:

* [Led Panel home page](http://www.acmesystems.it/ledpanel)



#Where to buy:

* [Acme Systems eShop](http://www.acmesystems.it/catalog_arietta)

#Licence terms

Copyright (C) 2015 Sergio Tanzilli, All Rights Reserved.
sergio@tanzilli.com 
http://www.acmesystems.it/ledpanel 

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

* [GNU GENERAL PUBLIC LICENSE](./LICENSE)
