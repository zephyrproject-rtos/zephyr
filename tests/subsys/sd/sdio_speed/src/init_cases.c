/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/ztest.h>

#include "speed_test.h"

ZTEST(sdio_speed, test_init_rejected_sdr50_skips_tuning)
{
	const struct init_case test = {
		.uhs_reg = 0x01U, .speed_write = 0x05U,
		.want_clock = 25000000U, .want_timing = SDHC_TIMING_SDR12,
	};

	sdio_speed_run_init_case(&test);
}

ZTEST(sdio_speed, test_init_rejected_sdr104_skips_tuning)
{
	const struct init_case test = {
		.uhs_reg = 0x02U, .speed_write = 0x07U,
		.want_clock = 25000000U, .want_timing = SDHC_TIMING_SDR12,
	};

	sdio_speed_run_init_case(&test);
}

ZTEST(sdio_speed, test_init_accepted_sdr50_tunes)
{
	const struct init_case test = {
		.uhs_reg = 0x01U, .speed_write = 0x05U, .accept = true,
		.want_clock = 100000000U, .want_timing = SDHC_TIMING_SDR50, .want_tunes = 1U,
	};

	sdio_speed_run_init_case(&test);
}

ZTEST(sdio_speed, test_init_accepted_sdr104_tunes)
{
	const struct init_case test = {
		.uhs_reg = 0x02U, .speed_write = 0x07U, .accept = true,
		.want_clock = 208000000U, .want_timing = SDHC_TIMING_SDR104, .want_tunes = 1U,
	};

	sdio_speed_run_init_case(&test);
}

ZTEST(sdio_speed, test_init_tuning_error_propagates)
{
	const struct init_case test = {
		.uhs_reg = 0x02U, .speed_write = 0x07U, .accept = true,
		.want_clock = 208000000U, .want_timing = SDHC_TIMING_SDR104, .want_tunes = 1U,
		.tuning_error = -EIO, .want_ret = -EIO,
	};

	sdio_speed_run_init_case(&test);
}
