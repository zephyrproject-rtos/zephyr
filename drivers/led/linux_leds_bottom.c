/*
 * SPDX-FileCopyrightText: Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <nsi_tracing.h>

int linux_led_open(const char *path, int *max)
{
	/* 34: Length of /sys/class/leds/%s/max_brightness + \0 */
	char full_path[strlen(path) + 34];
	char buf[32];
	int fd;
	int ret;

	ret = snprintf(full_path, sizeof(full_path), "/sys/class/leds/%s/max_brightness", path);
	if (ret < 0) {
		return ret;
	} else if (ret >= sizeof(full_path)) {
		return -1;
	}

	fd = open(full_path, O_RDONLY);
	if (fd < 0) {
		nsi_print_warning("Failed to open the led max brightness device %s: %s\n",
				  full_path, strerror(errno));
		return -1;
	}

	ret = read(fd, buf, sizeof(buf) - 1);
	if (ret < 0) {
		nsi_print_warning("Read error: %s", strerror(errno));
		close(fd);
		return -1;
	}

	buf[ret] = '\0';
	*max = strtol(buf, NULL, 10);

	close(fd);

	ret = snprintf(full_path, sizeof(full_path), "/sys/class/leds/%s/brightness", path);
	if (ret < 0) {
		return ret;
	} else if (ret >= sizeof(full_path)) {
		return -1;
	}

	fd = open(full_path, O_WRONLY);
	if (fd < 0) {
		nsi_print_warning("Failed to open the led device %s: %s\n",
				  full_path, strerror(errno));
		return -1;
	}

	return fd;
}
