#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXBUFFER_PER_PANEL 32*32*3
#define OUT_FILE "/sys/class/ledpanel/rgb_buffer"

// Write on rgb_buffer the buffer content
void WriteBuffer(unsigned char *buffer) {
	int fd;

	if ((fd=open(OUT_FILE,O_WRONLY))<0) {
		printf("open() error\n");
		return;
	}
	write(fd,buffer,MAXBUFFER_PER_PANEL);
	close(fd);
}

// Fill the buffer with a rgb color
void fill_full(unsigned char r,unsigned char g,unsigned char b,unsigned char *buffer, int show) {
	int i;
	
	for (i=0;i<MAXBUFFER_PER_PANEL;i+=3) {
		buffer[i+0]=r;
		buffer[i+1]=g;
		buffer[i+2]=b;
	}
	if (show) WriteBuffer(buffer);
}

void main(int argc, char *argv[]) {
	int i,r,g,b,x,y;
	unsigned char buffer[MAXBUFFER_PER_PANEL];

	fill_full(0,0,0,buffer,1);

	r=1;
	g=0;
	b=0;
	for (r=1;r<7;r++) {
		for (y=0;y<32;y++) {
			for (x=0;x<32;x++) {
				printf("[%d,%d]=%d %d %d\n",x,y,r,g,b);
				buffer[x*3+y*96+0]=r;	
				buffer[x*3+y*96+1]=g;	
				buffer[x*3+y*96+2]=b;	
				WriteBuffer(buffer);
				usleep(10000);
			}
		}
		fill_full(0,0,0,buffer,1);
	}	
}
