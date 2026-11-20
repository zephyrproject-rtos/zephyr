/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/ztest.h>

#include "speed_test.h"

ZTEST(sdio_speed, test_hs_1v8)
{
	const struct speed_case test = {
		ACCEPTED(SDIO_SUPPORT_HS, SD_VOL_1_8_V, 0x83U, 50000000U, SDHC_TIMING_SDR25)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_hs_1v8_host_cap)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_HS, .voltage = SD_VOL_1_8_V, .host_max = 40000000U,
		.initial_reg = 0x81U, .accepted_reg = 0x83U, .want_write_data = 0x83U,
		.accept_at = 1, .want_writes = 1, .want_sets = 1U,
		.want_clock = 40000000U, .want_timing = SDHC_TIMING_SDR25
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_hs_1v8_set_io_error)
{
	const struct speed_case test = {
		ACCEPTED(SDIO_SUPPORT_HS, SD_VOL_1_8_V, 0x83U, 50000000U, SDHC_TIMING_SDR25),
		.host_error = -EIO, .want_ret = -EIO
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_literal_03_hs_1v8_validating_host)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_HS, .voltage = SD_VOL_1_8_V, .host_max = 208000000U,
		.initial_reg = 0x01U, .accepted_reg = 0x03U, .want_write_data = 0x03U,
		.accept_at = 1, .want_writes = 1, .want_sets = 1U,
		.want_clock = 50000000U, .want_timing = SDHC_TIMING_SDR25,
		.validate_high_speed = true,
	};

	sdio_speed_run_case(&test);
}
