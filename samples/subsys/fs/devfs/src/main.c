/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdarg.h>
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>
#include <zephyr/fs/devfs.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/unistd.h>

int main(void)
{
	int fd = open("/dev/rs232", O_RDWR);
	write(fd, "Hello, World!", 13);
	close(fd);

	while (1) {
		k_msleep(1000);
	}

	return 0;
}
