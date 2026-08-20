/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LINUX_FUEL_GAUGE_BOTTOM_H
#define LINUX_FUEL_GAUGE_BOTTOM_H

/* Read an integer sysfs attribute: <base_path>/<attr> */
int linux_fuel_gauge_read(const char *base_path, const char *attr, long *value);

/* Read a string sysfs attribute: <base_path>/<attr> */
int linux_fuel_gauge_read_buffer(const char *base_path, const char *attr, char *buf,
				 size_t buf_size);

#endif /* LINUX_FUEL_GAUGE_BOTTOM_H */
