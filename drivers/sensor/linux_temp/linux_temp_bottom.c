/*
 * Copyright 2026 Bayrem Gharsellaoui
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nsi_tracing.h>

#include "linux_temp_bottom.h"

int linux_temp_read(const char *path, int32_t *temperature_mc)
{
	char buf[32];
	char *end;
	long value;
	ssize_t bytes_read;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		nsi_print_warning("Failed to open temperature file %s: %s\n", path,
				  strerror(errno));
		return -1;
	}

	bytes_read = read(fd, buf, sizeof(buf) - 1);
	(void)close(fd);

	if (bytes_read <= 0) {
		nsi_print_warning("Failed to read temperature file %s\n", path);
		return -1;
	}

	buf[bytes_read] = '\0';

	value = strtol(buf, &end, 10);
	if ((end == buf) || (value < INT32_MIN) || (value > INT32_MAX)) {
		nsi_print_warning("Invalid temperature value read from %s\n", path);
		return -1;
	}

	*temperature_mc = (int32_t)value;

	return 0;
}
