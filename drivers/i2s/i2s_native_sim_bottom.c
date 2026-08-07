/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <nsi_errno.h>
#include <nsi_tracing.h>

int ns_i2s_open_file_bottom(const char *pathname, int rx)
{
	int fd;

	if (rx) {
		fd = open(pathname, O_RDONLY);
	} else {
		fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	}

	if (fd < 0) {
		nsi_print_warning("%s could not be opened (%s)\n", pathname, strerror(errno));
		return -nsi_errno_to_mid(errno);
	}

	return fd;
}
