#define _GNU_SOURCE 1
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
	int fd, ret;
	struct statx before, after;

	if (argc < 2) {
		fprintf(stderr, "Must specify filename!\n");
		return 1;
	}

	fd = open(argv[1], O_RDWR|O_CREAT, 0600);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = statx(fd, "", AT_EMPTY_PATH, STATX_CTIME, &before);
	if (ret) {
		perror("statx");
		return 1;
	}

	sleep(1);

	ret = unlink(argv[1]);
	if (ret) {
		perror("unlink");
		return 1;
	}

	ret = statx(fd, "", AT_EMPTY_PATH, STATX_CTIME, &after);
	if (ret) {
		perror("statx");
		return 1;
	}

	if (before.stx_ctime.tv_nsec == after.stx_ctime.tv_nsec &&
	    before.stx_ctime.tv_sec == after.stx_ctime.tv_sec) {
		printf("No change to ctime after unlink!\n");
		return 1;
	}
	return 0;
}
