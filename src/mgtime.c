// SPDX-License-Identifier: GPL-2.0
/*
 * Older Linux kernels always use coarse-grained timestamps, with a
 * resolution of around 1 jiffy. Writes that are done in quick succession
 * on those kernels apparent change to the ctime or mtime.
 *
 * Newer kernels attempt to ensure that fine-grained timestamps are used
 * when the mtime or ctime are queried from userland.
 *
 * Open a file and do a 1 byte write to it and then statx the mtime and ctime.
 * Do that in a loop 1000 times and ensure that the value is different from
 * the last.
 *
 * Copyright (c) 2023: Jeff Layton <jlayton@kernel.org>
 */
#define _GNU_SOURCE 1
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <stdbool.h>

#define NUM_WRITES 1000

static int64_t statx_ts_cmp(struct statx_timestamp *ts1, struct statx_timestamp *ts2)
{
	int64_t ret = ts2->tv_sec - ts1->tv_sec;

	if (ret)
		return ret;

	ret = ts2->tv_nsec;
	ret -= ts1->tv_nsec;

	return ret;
}

int main(int argc, char **argv)
{
	int ret, fd, i;
	struct statx stx = { };
	struct statx_timestamp ctime, mtime;
	bool verbose = false;

	while ((i = getopt(argc, argv, "v")) != -1) {
		switch (i) {
		case 'v':
			verbose = true;
			break;
		}
	}

	if (argc < 2) {
		errno = -EINVAL;
		perror("usage");
	}

	fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	ret = statx(fd, "", AT_EMPTY_PATH, STATX_MTIME|STATX_CTIME, &stx);
	if (ret < 0) {
		perror("stat");
		return 1;
	}

	ctime = stx.stx_ctime;
	mtime = stx.stx_mtime;

	for (i = 0; i < NUM_WRITES; ++i) {
		ssize_t written;

		written = write(fd, "a", 1);
		if (written < 0) {
			perror("write");
			return 1;
		}

		ret = statx(fd, "", AT_EMPTY_PATH, STATX_MTIME|STATX_CTIME, &stx);
		if (ret < 0) {
			perror("stat");
			return 1;
		}

		if (verbose)
			printf("%d: %llu.%u\n", i, stx.stx_ctime.tv_sec, stx.stx_ctime.tv_nsec);

		if (!statx_ts_cmp(&ctime, &stx.stx_ctime)) {
			printf("Duplicate ctime after write!\n");
			return 1;
		}

		if (!statx_ts_cmp(&mtime, &stx.stx_mtime)) {
			printf("Duplicate mtime after write!\n");
			return 1;
		}

		ctime = stx.stx_ctime;
		mtime = stx.stx_mtime;
	}
	return 0;
}
