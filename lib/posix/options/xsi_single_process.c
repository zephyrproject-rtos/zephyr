/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Meta
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/clock.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(xsi_single_process, CONFIG_XSI_SINGLE_PROCESS_LOG_LEVEL);

extern int z_setenv(const char *name, const char *val, int overwrite);

long gethostid(void)
{
	int rc;
	uint32_t buf = 0;

	rc = hwinfo_get_device_id((uint8_t *)&buf, sizeof(buf));
	if ((rc < 0) || (rc != sizeof(buf)) || (buf == 0)) {
		LOG_DBG("%s() failed: %d", "hwinfo_get_device_id", rc);
		return (long)rc;
	}

	return (long)buf;
}

int gettimeofday(struct timeval *tv, void *tz)
{
	struct timespec ts;
	int res;

	/*
	 * As per POSIX, "if tzp is not a null pointer, the behavior is unspecified."  "tzp" is the
	 * "tz" parameter above.
	 */
	ARG_UNUSED(tz);

	res = sys_clock_gettime(SYS_CLOCK_REALTIME, &ts);
	if (res < 0) {
		LOG_DBG("%s() failed: %d", "sys_clock_gettime", res);
		errno = -res;
		return -1;
	}

	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / NSEC_PER_USEC;

	return 0;
}

int putenv(char *string)
{
	if (string == NULL) {
		errno = EINVAL;
		return -1;
	}

	char *const name = string;

	for (char *val = name; *val != '\0'; ++val) {
		if (*val == '=') {
			int rc;

			*val = '\0';
			++val;
			rc = z_setenv(name, val, 1);
			--val;
			*val = '=';

			if (rc < 0) {
				LOG_DBG("%s() failed: %d", "setenv", rc);
				return rc;
			}

			return 0;
		}
	}

	/* was unable to find '=' in string */
	errno = EINVAL;
	return -1;
}
