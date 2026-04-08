/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "nsi_host_io.h"

int nsi_host_open_read(const char *pathname)
{
	return open(pathname, O_RDONLY);
}

int nsi_host_open_write_truncate(const char *pathname)
{
	return open(pathname, O_CREAT | O_TRUNC | O_WRONLY, 0600);
}
