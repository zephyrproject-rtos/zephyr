/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sdhc_emul.h>
#include <zephyr/sd/sdio.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/ztest.h>

#include "sd_init.h"

static const struct device *const host = DEVICE_DT_GET(DT_NODELABEL(sdio_speed_host));
static unsigned int speed_write_count;
static uint32_t previous_speed_clock;
static enum sdhc_timing_mode previous_speed_timing;
static struct sd_card speed_card;
static struct sdhc_io host_io;
static uint32_t tuning_count;
static uint32_t accept_on_write;
static uint8_t requested_bss = SDIO_CCCR_SPEED_SDR25;

static int speed_request(const struct emul *target, struct sdhc_command *cmd,
			struct sdhc_data *data)
{
	uint32_t addr = (cmd->arg >> SDIO_CMD_ARG_REG_ADDR_SHIFT) & SDIO_CMD_ARG_REG_ADDR_MASK;
	uint32_t fn = (cmd->arg >> SDIO_CMD_ARG_FUNC_NUM_SHIFT) & 0x7U;

	ARG_UNUSED(target);
	ARG_UNUSED(data);

	if (cmd->opcode != SDIO_RW_DIRECT || fn != SDIO_FUNC_NUM_0 ||
	    addr != SDIO_CCCR_SPEED || !(cmd->arg & BIT(SDIO_CMD_ARG_RW_SHIFT))) {
		return -ENOSYS;
	}

	zassert_true(cmd->arg & BIT(SDIO_DIRECT_CMD_ARG_RAW_SHIFT));
	zassert_equal(cmd->arg & SDIO_CCCR_SPEED_MASK,
		      requested_bss << SDIO_CCCR_SPEED_SHIFT);
	if (speed_write_count++ == 0) {
		previous_speed_clock = speed_card.bus_io.clock;
		previous_speed_timing = speed_card.bus_io.timing;
	}

	if (accept_on_write != 0U && speed_write_count >= accept_on_write) {
		return -ENOSYS;
	}

	cmd->response[0] = SDIO_CCCR_SPEED_SHS;
	return 0;
}

static int observe_io(const struct emul *target, const struct sdhc_io *ios)
{
	ARG_UNUSED(target);

	host_io = *ios;
	return -ENOSYS;
}

static int observe_tuning(const struct emul *target)
{
	ARG_UNUSED(target);

	zassert_true(host_io.timing == SDHC_TIMING_SDR50 ||
		     host_io.timing == SDHC_TIMING_SDR104);
	tuning_count++;
	return -ENOSYS;
}

static struct sdhc_emul_api speed_api = {
	.request = speed_request,
	.set_io = observe_io,
	.execute_tuning = observe_tuning,
};

static void check_host(uint32_t clock, enum sdhc_timing_mode timing,
		       enum sd_voltage voltage, uint32_t tunings)
{
	zassert_equal(host_io.clock, clock);
	zassert_equal(host_io.timing, timing);
	zassert_equal(host_io.signal_voltage, voltage);
	zassert_equal(tuning_count, tunings);
}

static void seed_high_speed_support(void)
{
	struct sdhc_command cmd = {
		.opcode = SDIO_RW_DIRECT,
		.arg = BIT(SDIO_CMD_ARG_RW_SHIFT) |
		       (SDIO_CCCR_SPEED << SDIO_CMD_ARG_REG_ADDR_SHIFT) | SDIO_CCCR_SPEED_SHS,
		.response_type = SD_RSP_TYPE_R5,
		.timeout_ms = 100,
	};

	zassert_true(device_is_ready(host));
	zassert_ok(sdhc_request(host, &cmd, NULL));
	speed_card = (struct sd_card){0};
	host_io = (struct sdhc_io){0};
	tuning_count = 0U;
	speed_write_count = 0U;
	accept_on_write = 1U;
	requested_bss = SDIO_CCCR_SPEED_SDR25;
	sdhc_emul_get_card(host)->mock_api = &speed_api;
}

ZTEST(sdio_emul_speed, test_card_rejects_high_speed)
{
	struct sdhc_emul *emulator = sdhc_emul_get_card(host);
	struct sdio_func fn0 = {.num = SDIO_FUNC_NUM_0, .card = &speed_card};
	uint8_t speed;
	int ret;

	zassert_not_null(emulator);
	seed_high_speed_support();
	speed_write_count = 0;
	accept_on_write = 0U;
	ret = sd_init(host, &speed_card);
	emulator->mock_api = NULL;

	zassert_ok(ret);
	zassert_equal(previous_speed_clock, 400000U);
	zassert_equal(speed_card.bus_io.clock, SD_CLOCK_25MHZ);
	zassert_equal(speed_card.bus_io.timing, previous_speed_timing);
	check_host(SD_CLOCK_25MHZ, SDHC_TIMING_LEGACY, SD_VOL_3_3_V, 0U);
	zassert_equal(speed_write_count, CONFIG_SD_RETRY_COUNT + 1U);
	zassert_ok(sdio_read_byte(&fn0, SDIO_CCCR_SPEED, &speed));
	zassert_equal(speed, SDIO_CCCR_SPEED_SHS);
}

ZTEST(sdio_emul_speed, test_card_accepts_high_speed)
{
	struct sdio_func fn0 = {.num = SDIO_FUNC_NUM_0, .card = &speed_card};
	uint8_t speed;

	seed_high_speed_support();
	zassert_ok(sd_init(host, &speed_card));
	zassert_equal(speed_card.bus_io.clock, 50000000U);
	zassert_equal(speed_card.bus_io.timing, SDHC_TIMING_HS);
	check_host(SD_CLOCK_50MHZ, SDHC_TIMING_HS, SD_VOL_3_3_V, 0U);
	zassert_equal(speed_write_count, 1U);
	zassert_ok(sdio_read_byte(&fn0, SDIO_CCCR_SPEED, &speed));
	zassert_equal(speed & SDIO_CCCR_SPEED_MASK,
		      SDIO_CCCR_SPEED_SDR25 << SDIO_CCCR_SPEED_SHIFT);
}

static void run_uhs_initialization(uint8_t bss, uint8_t uhs, uint32_t accept_on,
				   uint32_t clock, enum sdhc_timing_mode timing)
{
	struct sdhc_emul *emulator = sdhc_emul_get_card(host);
	struct sdio_func fn0 = {.num = SDIO_FUNC_NUM_0, .card = &speed_card};
	int ret;

	seed_high_speed_support();
	/* Use the real emulator after voltage negotiation, without physical CMD11. */
	speed_card.sdhc = host;
	speed_card.type = CARD_SDIO;
	speed_card.flags = SD_1800MV_FLAG;
	speed_card.bus_io.clock = 400000U;
	speed_card.bus_io.signal_voltage = SD_VOL_1_8_V;
	speed_card.bus_io.timing = SDHC_TIMING_SDR12;
	speed_card.host_props.f_max = 208000000U;
	speed_card.host_props.host_caps.vol_180_support = true;
	speed_card.host_props.host_caps.high_spd_support = true;
	speed_card.host_props.host_caps.sdr50_support = true;
	speed_card.host_props.host_caps.sdr104_support = true;
	speed_card.host_props.bus_4_bit_support = true;
	zassert_ok(sdio_write_byte(&fn0, 0U, 0x33U));
	zassert_ok(sdio_write_byte(&fn0, SDIO_CCCR_UHS, uhs));
	requested_bss = bss;
	speed_write_count = 0;
	accept_on_write = accept_on;
	ret = sdio_card_init(&speed_card);

	emulator->mock_api = NULL;
	requested_bss = SDIO_CCCR_SPEED_SDR25;
	zassert_ok(ret);
	zassert_equal(speed_card.bus_io.clock, clock);
	zassert_equal(speed_card.bus_io.timing, timing);
	check_host(clock, timing, SD_VOL_1_8_V,
		   timing == SDHC_TIMING_SDR50 || timing == SDHC_TIMING_SDR104 ? 1U : 0U);
	zassert_equal(speed_write_count, accept_on != 0U ? accept_on : CONFIG_SD_RETRY_COUNT + 1U);
}

ZTEST(sdio_emul_speed, test_rejected_sdr50_initialization)
{
	run_uhs_initialization(SDIO_CCCR_SPEED_SDR50, SDIO_CCCR_UHS_SDR50, 0U,
			       SD_CLOCK_25MHZ, SDHC_TIMING_SDR12);
}

ZTEST(sdio_emul_speed, test_rejected_sdr104_initialization)
{
	run_uhs_initialization(SDIO_CCCR_SPEED_SDR104, SDIO_CCCR_UHS_SDR104, 0U,
			       SD_CLOCK_25MHZ, SDHC_TIMING_SDR12);
}

ZTEST(sdio_emul_speed, test_accepted_sdr50_initialization)
{
	run_uhs_initialization(SDIO_CCCR_SPEED_SDR50, SDIO_CCCR_UHS_SDR50, 1U,
			       SD_CLOCK_100MHZ, SDHC_TIMING_SDR50);
}

ZTEST(sdio_emul_speed, test_accepted_sdr104_initialization)
{
	run_uhs_initialization(SDIO_CCCR_SPEED_SDR104, SDIO_CCCR_UHS_SDR104, 1U,
			       SD_CLOCK_208MHZ, SDHC_TIMING_SDR104);
}

ZTEST(sdio_emul_speed, test_sdr104_accepted_on_last_attempt)
{
	run_uhs_initialization(SDIO_CCCR_SPEED_SDR104, SDIO_CCCR_UHS_SDR104,
			       CONFIG_SD_RETRY_COUNT + 1U, SD_CLOCK_208MHZ, SDHC_TIMING_SDR104);
}

ZTEST(sdio_emul_speed, test_accepted_sdr25_initialization)
{
	run_uhs_initialization(SDIO_CCCR_SPEED_SDR25, 0U, 1U,
			       SD_CLOCK_50MHZ, SDHC_TIMING_SDR25);
}

ZTEST(sdio_emul_speed, test_rejected_sdr25_initialization)
{
	run_uhs_initialization(SDIO_CCCR_SPEED_SDR25, 0U, 0U,
			       SD_CLOCK_25MHZ, SDHC_TIMING_SDR12);
}

static void speed_after(void *fixture)
{
	ARG_UNUSED(fixture);

	sdhc_emul_get_card(host)->mock_api = NULL;
}

ZTEST_SUITE(sdio_emul_speed, NULL, NULL, NULL, speed_after, NULL);
