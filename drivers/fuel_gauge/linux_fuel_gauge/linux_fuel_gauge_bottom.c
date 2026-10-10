/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <nsi_errno.h>
#include <nsi_tracing.h>

#define POWER_SUPPLY_NODE "/sys/class/power_supply"

int linux_fuel_gauge_read_buffer(const char *base_path, const char *attr, char *buf,
				 size_t buf_size)
{
	char path[sizeof(POWER_SUPPLY_NODE) + strlen(base_path) + strlen(attr) + 2];
	int fd;
	int ret;
	int err;

	(void)snprintf(path, sizeof(path), POWER_SUPPLY_NODE "/%s/%s", base_path, attr);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		err = errno;
		/* A battery is not required to provide every attribute. */
		if (err != ENOENT) {
			nsi_print_warning("Failed to open %s: %s\n", path, strerror(err));
		}
		return -nsi_errno_to_mid(err);
	}

	ret = read(fd, buf, buf_size - 1);
	if (ret <= 0) {
		err = (ret < 0) ? errno : EIO;
		nsi_print_warning("Read error on %s: %s\n", path,
				  (ret < 0) ? strerror(err) : "no data");
		close(fd);
		return -nsi_errno_to_mid(err);
	}

	close(fd);

	if (buf[ret - 1] == '\n') {
		ret--;
	}

	buf[ret] = '\0';

	return 0;
}

int linux_fuel_gauge_read(const char *base_path, const char *attr, int64_t *value)
{
	char buf[32];
	char *end;
	long long val;
	int ret;

	ret = linux_fuel_gauge_read_buffer(base_path, attr, buf, sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	errno = 0;
	val = strtoll(buf, &end, 10);
	if (errno == ERANGE) {
		nsi_print_warning("Value of %s/%s out of range: %s\n", base_path, attr, buf);
		return -nsi_errno_to_mid(ERANGE);
	}

	if (end == buf || *end != '\0') {
		nsi_print_warning("Value of %s/%s is not a number: %s\n", base_path, attr, buf);
		return -nsi_errno_to_mid(EINVAL);
	}

	*value = val;

	return 0;
}
