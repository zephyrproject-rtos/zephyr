/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <nsi_tracing.h>
#include <string.h>
#include <unistd.h>

#include "linux_evdev_bottom.h"

int linux_evdev_read(int fd, uint16_t *type, uint16_t *code, int32_t *value)
{
	struct input_event ev;
	int ret;

	ret = read(fd, &ev, sizeof(ev));
	if (ret < 0) {
		if (errno == EAGAIN || errno == EINTR) {
			return NATIVE_LINUX_EVDEV_NO_DATA;
		}
		nsi_print_warning("Read error: %s", strerror(errno));
		return -EIO;
	} else if (ret < sizeof(ev)) {
		nsi_print_warning("Unexpected read size: %d, expecting %d",
				  ret, sizeof(ev));
		return -EIO;
	}

	*type = ev.type;
	*code = ev.code;
	*value = ev.value;

	return 0;
}

int linux_evdev_open(const char *path)
{
	int fd;

	fd = open(path, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		nsi_print_error_and_exit(
				"Failed to open the evdev device %s: %s\n",
				path, strerror(errno));
	}

	return fd;
}
