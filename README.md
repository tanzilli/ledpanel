#Simple Linux driver for a 32x32 RGB led panel

Bit banging driver to manage a RGB led panel using the  
[Arieta G25 Linux Embedded SoM](http://www.acmesystems.it/arietta)

Project documentation:

* [Led Panel home page](http://www.acmesystems.it/ledpanel)
* [Video: Sliding clock](http://www.youtube.com/embed/Qszwey7jYl4)

##How to install

Create a directory called __ledpanel__ inside __linux/drivers__ 

Make a clone of the ledpanel git repository:

	~/linux/drivers$ git clone git://github.com/tanzilli/ledpanel.git

Add this line in __linux/driver/Makefile__:

	obj-$(CONFIG_LEDPANEL)   += ledpanel/

Add this line in__linux/driver/Kconfig__:

	source "drivers/ledpanel/Kconfig"

Launch __make menuconfig__ and

Enable the ledpanel driver:

	Device Drivers  --->
		<M> RGB led panel bit banging driver (NEW) 

Enable the High Resolution Timer Support:

	General setup  --->
		Timers subsystem  --->
			[*] High Resolution Timer Support  

#Where to buy:

* [Acme Systems eShop](http://www.acmesystems.it/catalog_arietta)

(c) 2015 - Sergio Tanzilli - sergio@tanzilli.com
