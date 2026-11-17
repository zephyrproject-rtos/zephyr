/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sdhc_emul.h>
#include <zephyr/sd/sdio.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/ztest.h>

static const struct device *const host = DEVICE_DT_GET(DT_NODELABEL(sdio_speed_host));
static unsigned int speed_write_count;
static uint32_t previous_speed_clock;
static enum sdhc_timing_mode previous_speed_timing;
static struct sd_card speed_card;

static int reject_speed(const struct emul *target, struct sdhc_command *cmd,
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
		      SDIO_CCCR_SPEED_SDR25 << SDIO_CCCR_SPEED_SHIFT);
	if (speed_write_count++ == 0) {
		previous_speed_clock = speed_card.bus_io.clock;
		previous_speed_timing = speed_card.bus_io.timing;
	}

	cmd->response[0] = SDIO_CCCR_SPEED_SHS;
	return 0;
}

static struct sdhc_emul_api reject_speed_api = {
	.request = reject_speed,
};

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
	emulator->mock_api = &reject_speed_api;
	ret = sd_init(host, &speed_card);
	emulator->mock_api = NULL;

	zassert_ok(ret);
	zassert_equal(speed_card.bus_io.clock, previous_speed_clock);
	zassert_equal(speed_card.bus_io.timing, previous_speed_timing);
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
	zassert_ok(sdio_read_byte(&fn0, SDIO_CCCR_SPEED, &speed));
	zassert_equal(speed & SDIO_CCCR_SPEED_MASK,
		      SDIO_CCCR_SPEED_SDR25 << SDIO_CCCR_SPEED_SHIFT);
}

ZTEST_SUITE(sdio_emul_speed, NULL, NULL, NULL, NULL, NULL);
