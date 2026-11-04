/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Deere & Company
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tle927x_hwinfo

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mfd/tle927x.h>

#define HWINFO_TLE927X_STATUS_CLEAR 0x00U

#define HWINFO_TLE927X_SUPPORTED                                                                   \
	(RESET_POR | RESET_WATCHDOG | RESET_BROWNOUT | RESET_TEMPERATURE | RESET_HARDWARE)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Only one infineon,tle927x-hwinfo instance is supported");

static uint32_t hwinfo_tle927x_cause;

static uint32_t hwinfo_tle927x_decode(uint8_t sup, uint8_t therm, uint8_t dev_stat)
{
	uint32_t flags = 0U;

	if ((sup & TLE927X_SUP_STAT_POR) != 0U) {
		flags = RESET_POR;
	} else {
		if (FIELD_GET(TLE927X_DEV_STAT_WD_FAIL, dev_stat) != 0U) {
			flags |= RESET_WATCHDOG;
		}

		if ((sup & TLE927X_SUP_STAT_VCC1_UV) != 0U) {
			flags |= RESET_BROWNOUT;
		}

		if ((therm & TLE927X_THERM_STAT_TSD2) != 0U) {
			flags |= RESET_TEMPERATURE;
		}

		if ((flags == 0U) && (FIELD_GET(TLE927X_DEV_STAT_PREV, dev_stat) ==
				      TLE927X_PREV_RESTART_FAILURE)) {
			flags = RESET_HARDWARE;
		}
	}

	return flags;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	if (cause == NULL) {
		return -EINVAL;
	}

	*cause = hwinfo_tle927x_cause;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	hwinfo_tle927x_cause = 0U;

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	if (supported == NULL) {
		return -EINVAL;
	}

	*supported = HWINFO_TLE927X_SUPPORTED;

	return 0;
}

static int hwinfo_tle927x_init(void)
{
	const struct device *parent = DEVICE_DT_GET(DT_INST_PARENT(0));
	const uint8_t reset_regs[] = {
		TLE927X_REG_SUP_STAT,
		TLE927X_REG_THERM_STAT,
		TLE927X_REG_DEV_STAT,
	};
	uint8_t sup, therm, dev_stat;
	size_t i;
	int ret;

	if (!device_is_ready(parent)) {
		return -ENODEV;
	}

	ret = mfd_tle927x_read_reg(parent, TLE927X_REG_SUP_STAT, &sup);
	if (ret != 0) {
		return ret;
	}

	ret = mfd_tle927x_read_reg(parent, TLE927X_REG_THERM_STAT, &therm);
	if (ret != 0) {
		return ret;
	}

	ret = mfd_tle927x_read_reg(parent, TLE927X_REG_DEV_STAT, &dev_stat);
	if (ret != 0) {
		return ret;
	}

	for (i = 0U; i < ARRAY_SIZE(reset_regs); i++) {
		ret = mfd_tle927x_write_reg(parent, reset_regs[i], HWINFO_TLE927X_STATUS_CLEAR);
		if (ret != 0) {
			return ret;
		}
	}

	hwinfo_tle927x_cause = hwinfo_tle927x_decode(sup, therm, dev_stat);

	return 0;
}

SYS_INIT(hwinfo_tle927x_init, POST_KERNEL, CONFIG_HWINFO_TLE927X_INIT_PRIORITY);
