/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Muhammad Waleed Badar
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

#define POWER_SUPPLY_NODE "/sys/class/power_supply"

int linux_fuel_gauge_read(const char *base_path, const char *attr, long *value)
{
	char path[sizeof(POWER_SUPPLY_NODE) + strlen(base_path) + strlen(attr) + 2];
	char buf[32];
	int fd;
	int ret;

	ret = snprintf(path, sizeof(path), POWER_SUPPLY_NODE "/%s/%s", base_path, attr);
	if (ret < 0) {
		return ret;
	} else if (ret >= sizeof(path)) {
		return -1;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		nsi_print_warning("Failed to open %s: %s\n", path, strerror(errno));
		return -1;
	}

	ret = read(fd, buf, sizeof(buf) - 1);
	if (ret < 0) {
		nsi_print_warning("Read error on %s: %s\n", path, strerror(errno));
		close(fd);
		return -1;
	}

	buf[ret] = '\0';
	*value = strtol(buf, NULL, 10);

	close(fd);

	return 0;
}

int linux_fuel_gauge_read_buffer(const char *base_path, const char *attr, char *buf,
				 size_t buf_size)
{
	char path[sizeof(POWER_SUPPLY_NODE) + strlen(base_path) + strlen(attr) + 2];
	int fd;
	int ret;
	size_t len;

	ret = snprintf(path, sizeof(path), POWER_SUPPLY_NODE "/%s/%s", base_path, attr);
	if (ret < 0) {
		return ret;
	} else if (ret >= sizeof(path)) {
		return -1;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		nsi_print_warning("Failed to open %s: %s\n", path, strerror(errno));
		return -1;
	}

	ret = read(fd, buf, buf_size - 1);
	close(fd);
	if (ret < 0) {
		nsi_print_warning("Read error on %s: %s\n", path, strerror(errno));
		return -1;
	}

	buf[ret] = '\0';

	len = strlen(buf);
	if (len > 0 && buf[len - 1] == '\n') {
		buf[len - 1] = '\0';
	}

	return 0;
}
