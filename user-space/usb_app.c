#include <fcntl.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

	int fd = open("/dev/bbb_media_player0", O_RDWR);

	if (fd < 0) {
		printf("Cannot open bbb_media_player\n");
		return -1;
	}

	return 0;
}