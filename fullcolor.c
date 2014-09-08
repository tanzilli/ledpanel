// Send a fullcolor pattern to
// ledpanel rgb_buffer 

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXBUFFER_PER_PANEL 32*32*3
#define OUT_FILE "/sys/class/ledpanel/rgb_buffer"

void WriteBuffer(unsigned char *buffer) {
	int fd;

	if ((fd=open(OUT_FILE,O_WRONLY))<0) {
		printf("open() error\n");
		return;
	}
	
	write(fd,buffer,MAXBUFFER_PER_PANEL);
	close(fd);
}

void Full(unsigned char r,unsigned char g,unsigned char b,unsigned char *buffer, int show) {
	int i;
	
	for (i=0;i<MAXBUFFER_PER_PANEL;i+=3) {
		buffer[i+0]=r;
		buffer[i+1]=g;
		buffer[i+2]=b;
	}
	if (show) WriteBuffer(buffer);
}

#define PAUSE 500000 //us

void main(void) {
	int i;
	unsigned char buffer[MAXBUFFER_PER_PANEL];
	
	Full(0,0,0,buffer,0);	

	for (i=0;i<100;i++) {
		// RED
		Full(1,0,0,buffer,1);	
		usleep(PAUSE);

		// GREEN
		Full(0,1,0,buffer,1);	
		usleep(PAUSE);

		// BLUE
		Full(0,0,1,buffer,1);	
		usleep(PAUSE);

		// YELLOW
		Full(1,1,0,buffer,1);	
		usleep(PAUSE);

		// CYAN
		Full(0,1,1,buffer,1);	
		usleep(PAUSE);

		// WHITE
		Full(1,1,1,buffer,1);	
		usleep(PAUSE);
	}
}
