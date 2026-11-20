/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/ztest.h>

#include "speed_test.h"

ZTEST(sdio_speed, test_rejected_hs)
{
	const struct speed_case test = {
		REJECTED(SDIO_SUPPORT_HS, SD_VOL_3_3_V, 0x83U, 0x81U, SDHC_TIMING_LEGACY)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_rejected_sdr50)
{
	const struct speed_case test = {
		REJECTED(SDIO_SUPPORT_SDR50, SD_VOL_1_8_V, 0x85U, 0x81U, SDHC_TIMING_SDR12)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_rejected_sdr104)
{
	const struct speed_case test = {
		REJECTED(SDIO_SUPPORT_SDR104, SD_VOL_1_8_V, 0x87U, 0x81U, SDHC_TIMING_SDR12)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_rejected_ddr50)
{
	const struct speed_case test = {
		REJECTED(SDIO_SUPPORT_DDR50, SD_VOL_1_8_V, 0x89U, 0x81U, SDHC_TIMING_SDR12)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_wrong_nonzero_bss)
{
	const struct speed_case test = {
		REJECTED(SDIO_SUPPORT_SDR104, SD_VOL_1_8_V, 0x87U, 0x83U, SDHC_TIMING_SDR12)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_default_rejected)
{
	const struct speed_case test = {
		.voltage = SD_VOL_3_3_V, .host_max = 208000000U,
		.initial_reg = 0x80U, .rejected_reg = 0x82U, .want_write_data = 0x80U,
		.want_writes = -1, .want_clock = 25000000U, .want_timing = SDHC_TIMING_LEGACY
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_immediate_sdr50)
{
	const struct speed_case test = {
		ACCEPTED(SDIO_SUPPORT_SDR50, SD_VOL_1_8_V, 0x85U, 100000000U, SDHC_TIMING_SDR50)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_immediate_sdr104)
{
	const struct speed_case test = {
		ACCEPTED(SDIO_SUPPORT_SDR104, SD_VOL_1_8_V, 0x87U, 208000000U, SDHC_TIMING_SDR104)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_immediate_ddr50)
{
	const struct speed_case test = {
		ACCEPTED(SDIO_SUPPORT_DDR50, SD_VOL_1_8_V, 0x89U, 50000000U, SDHC_TIMING_DDR50)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_sdr50_accepted_shs_clear)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_SDR50, .voltage = SD_VOL_1_8_V, .host_max = 208000000U,
		.initial_reg = 0x81U, .accepted_reg = 0x04U, .want_write_data = 0x85U,
		.accept_at = 1, .want_writes = 1, .want_sets = 1U,
		.want_clock = 100000000U, .want_timing = SDHC_TIMING_SDR50
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_final_sdr104)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_SDR104, .voltage = SD_VOL_1_8_V, .host_max = 208000000U,
		.initial_reg = 0x81U, .rejected_reg = 0x81U, .accepted_reg = 0x87U,
		.want_write_data = 0x87U, .accept_at = -1, .want_writes = -1, .want_sets = 1U,
		.want_clock = 208000000U, .want_timing = SDHC_TIMING_SDR104
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_delayed_sdr50)
{
	if (CONFIG_SD_RETRY_COUNT == 0) {
		ztest_test_skip();
	}
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_SDR50, .voltage = SD_VOL_1_8_V, .host_max = 208000000U,
		.initial_reg = 0x8FU, .rejected_reg = 0xC1U, .accepted_reg = 0xC5U,
		.want_write_data = 0x85U, .accept_at = 2, .want_writes = 2, .want_sets = 1U,
		.want_clock = 100000000U, .want_timing = SDHC_TIMING_SDR50,
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_control_hs_3v3)
{
	const struct speed_case test = {
		ACCEPTED(SDIO_SUPPORT_HS, SD_VOL_3_3_V, 0x83U, 50000000U, SDHC_TIMING_HS)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_control_hs_3v0)
{
	const struct speed_case test = {
		ACCEPTED(SDIO_SUPPORT_HS, SD_VOL_3_0_V, 0x83U, 50000000U, SDHC_TIMING_HS)
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_control_default_accepted)
{
	const struct speed_case test = {
		.voltage = SD_VOL_3_3_V, .host_max = 208000000U,
		.initial_reg = 0x80U, .accepted_reg = 0x80U, .want_write_data = 0x80U,
		.accept_at = 1, .want_writes = 1, .want_sets = 1U,
		.want_clock = 25000000U, .want_timing = SDHC_TIMING_LEGACY
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_control_host_cap)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_HS, .voltage = SD_VOL_3_3_V, .host_max = 40000000U,
		.initial_reg = 0x81U, .accepted_reg = 0x83U, .want_write_data = 0x83U,
		.accept_at = 1, .want_writes = 1, .want_sets = 1U,
		.want_clock = 40000000U, .want_timing = SDHC_TIMING_HS
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_control_read_error)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_HS, .voltage = SD_VOL_3_3_V, .host_max = 208000000U,
		.initial_reg = 0x81U, .read_error = -EIO, .want_ret = -EIO,
		.want_clock = 25000000U, .want_timing = SDHC_TIMING_LEGACY
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_control_write_error)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_HS, .voltage = SD_VOL_3_3_V, .host_max = 208000000U,
		.initial_reg = 0x81U, .want_write_data = 0x83U,
		.write_error = -ETIMEDOUT, .write_error_at = 1, .want_ret = -ETIMEDOUT,
		.want_writes = 1, .want_clock = 25000000U, .want_timing = SDHC_TIMING_LEGACY
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_control_second_write_error)
{
	if (CONFIG_SD_RETRY_COUNT == 0) {
		ztest_test_skip();
	}
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_SDR50, .voltage = SD_VOL_1_8_V, .host_max = 208000000U,
		.initial_reg = 0x81U, .rejected_reg = 0x81U, .want_write_data = 0x85U,
		.write_error = -ETIMEDOUT, .write_error_at = 2, .want_ret = -ETIMEDOUT,
		.want_writes = 2, .want_clock = 25000000U, .want_timing = SDHC_TIMING_SDR12,
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_control_set_io_error)
{
	const struct speed_case test = {
		ACCEPTED(SDIO_SUPPORT_HS, SD_VOL_3_3_V, 0x83U, 50000000U, SDHC_TIMING_HS),
		.host_error = -EIO, .want_ret = -EIO
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_literal_01_rejected_hs)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_HS, .voltage = SD_VOL_3_3_V, .host_max = 208000000U,
		.initial_reg = 0x01U, .rejected_reg = 0x01U, .want_write_data = 0x03U,
		.want_writes = -1, .want_clock = 25000000U, .want_timing = SDHC_TIMING_LEGACY,
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_literal_01_rejected_sdr50)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_SDR50, .voltage = SD_VOL_1_8_V, .host_max = 208000000U,
		.initial_reg = 0x01U, .rejected_reg = 0x01U, .want_write_data = 0x05U,
		.want_writes = -1, .want_clock = 25000000U, .want_timing = SDHC_TIMING_SDR12,
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_literal_05_immediate_sdr50)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_SDR50, .voltage = SD_VOL_1_8_V, .host_max = 208000000U,
		.initial_reg = 0x01U, .accepted_reg = 0x05U, .want_write_data = 0x05U,
		.accept_at = 1, .want_writes = 1, .want_sets = 1U,
		.want_clock = 100000000U, .want_timing = SDHC_TIMING_SDR50,
	};

	sdio_speed_run_case(&test);
}

ZTEST(sdio_speed, test_literal_07_final_sdr104)
{
	const struct speed_case test = {
		.caps = SDIO_SUPPORT_SDR104, .voltage = SD_VOL_1_8_V, .host_max = 208000000U,
		.initial_reg = 0x01U, .rejected_reg = 0x01U, .accepted_reg = 0x07U,
		.want_write_data = 0x07U, .accept_at = -1, .want_writes = -1, .want_sets = 1U,
		.want_clock = 208000000U, .want_timing = SDHC_TIMING_SDR104,
	};

	sdio_speed_run_case(&test);
}

ZTEST_SUITE(sdio_speed, NULL, NULL, NULL, NULL, NULL);
