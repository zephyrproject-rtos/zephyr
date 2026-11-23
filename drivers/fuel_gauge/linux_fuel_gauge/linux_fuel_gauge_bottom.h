/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LINUX_FUEL_GAUGE_BOTTOM_H
#define LINUX_FUEL_GAUGE_BOTTOM_H

#include <stddef.h>
#include <stdint.h>

/*
 * Both functions return 0 on success and a negative intermediate errno value
 * (see nsi_errno.h) on failure: the host error when the attribute cannot be
 * opened or read, EINVAL when it does not hold a number and ERANGE when the
 * number does not fit.
 */

/* Read the integer sysfs attribute <dir>/<attr> of a power supply */
int linux_fuel_gauge_read(const char *dir, const char *attr, int64_t *value);

/* Read the string sysfs attribute <dir>/<attr> of a power supply */
int linux_fuel_gauge_read_buffer(const char *dir, const char *attr, char *buf, size_t buf_size);

#endif /* LINUX_FUEL_GAUGE_BOTTOM_H */
