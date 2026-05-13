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

#include "i2s_native_sim_bottom.h"

int ns_i2s_open_rx_file_bottom(const char *pathname)
{
	int fd = open(pathname, O_RDONLY);

	if (fd < 0) {
		nsi_print_warning("%s could not be opened (%s)\n", pathname,
				  strerror(errno));
		return -nsi_errno_to_mid(errno);
	}

	return fd;
}

int ns_i2s_open_tx_file_bottom(const char *pathname)
{
	int fd = open(pathname, O_CREAT | O_TRUNC | O_WRONLY, 0600);

	if (fd < 0) {
		nsi_print_warning("%s could not be opened (%s)\n", pathname,
				  strerror(errno));
		return -nsi_errno_to_mid(errno);
	}

	return fd;
}
