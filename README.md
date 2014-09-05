#Simple Linux driver for a RGB led panel

My first attempt to write a driver for Linux to manage a Led Panel:

* [http://www.acmesystems.it/P6LED3232](http://www.acmesystems.it/P6LED3232)

##How to compile this module for Arietta G25

* [Install the Cross compiler toolchain on your Linux PC](http://www.acmesystems.it/compile_linux_3_16)

Clone this repository on your PC:

<code>
$ git clone https://github.com/tanzilli/ledpanel.git
$ cd ledpanel
</code>

Compile the module:

<code>
$ make -C ~/linux-3.16.1 ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- M=`pwd` modules
</code>

Save the module inside your Arietta microSD:

<code>
$ sshpass -p acmesystems ledpanel.ko root@192.168.10.10:/root
</code>

Open a command session with Arietta and load the Kernel module:

<code>
root@arietta:~# insmod ledpanel.ko
</code>

##Kernel config 

It is requested to increase the jiffies per second from 128 to 1024
inside the Kernel make menuconfig:

<code>
System Type  ---> 
  Atmel AT91 System-on-Chip  --->
    (<b>1024</b>) Kernel HZ (jiffies per second)
</code> 
    
