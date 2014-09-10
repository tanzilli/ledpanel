// Send a fullcolor pattern to
// ledpanel rgb_buffer 

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
	int i,r,g,b;
	unsigned char buffer[MAXBUFFER_PER_PANEL];

    if (argc!=4) {
        printf( "Use: %s r g b\n", argv[0] );
    } else {
		r=atoi(argv[1]);
		g=atoi(argv[2]);
		b=atoi(argv[3]);
		fill_full(r,g,b,buffer,1);
	}		
}
