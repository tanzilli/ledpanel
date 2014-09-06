/* Basic test program */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXBUFFER_PER_PANEL 32*32*3
//#define OUT_FILE "/sys/class/ledpanel/buffer32x32"
#define OUT_FILE "prova.hex"

void main(void) {
	int fd,i;
	unsigned char buffer[MAXBUFFER_PER_PANEL];
	
	for (i=0;i<MAXBUFFER_PER_PANEL;i+=3) {
		buffer[i+0]=0;
		buffer[i+1]=0;
		buffer[i+2]=0;
	}
	
	buffer[0]=1;
	
	if ((fd=open(OUT_FILE,O_WRONLY))<0) {
		printf("open() error\n");
		return;
	}
	
	write(fd,buffer,MAXBUFFER_PER_PANEL);
	
	close(fd);
}
