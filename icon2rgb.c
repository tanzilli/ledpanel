// http://fileformats.wikia.com/wiki/Icon

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MAX_ICON_LEN 4148

void main(int argc, char *argv[]) {
	int fd,len;
	unsigned char icon_buffer[MAX_ICON_LEN];
	
    if (argc!=3) {
        printf( "Use: %s source.ico destination.rgb\n", argv[0] );
		return;
    } else {
		fd = open(argv[1],O_RDONLY);
		len=read(fd,icon_buffer,MAX_ICON_LEN);
		printf("%d\n",len);
		
		
		close(fd);
	}
}		
