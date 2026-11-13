/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/fff.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/ztest.h>

#include "speed_test.h"
#include "sdio.c"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, fake_sdhc_request, const struct device *, struct sdhc_command *,
		struct sdhc_data *);
FAKE_VALUE_FUNC(int, fake_sdhc_set_io, const struct device *, struct sdhc_io *);
FAKE_VALUE_FUNC(int, fake_sdhc_execute_tuning, const struct device *);

static DEVICE_API(sdhc, fake_sdhc_api) = {
	.request = fake_sdhc_request,
	.set_io = fake_sdhc_set_io,
	.execute_tuning = fake_sdhc_execute_tuning,
};

DEVICE_DEFINE(sdio_speed_host, "sdio-speed-host", NULL, NULL, NULL, NULL,
	      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &fake_sdhc_api);

struct speed_fixture {
	const struct speed_case *test;
	uint32_t reads;
	uint32_t writes;
	uint8_t last_readback;
	bool card_accepted;
	struct sdhc_io requested_io;
};

static struct speed_fixture fixture;

static int request(const struct device *dev, struct sdhc_command *cmd,
		   struct sdhc_data *data)
{
	const struct speed_case *test = fixture.test;
	uint32_t read_arg = 0x13U << 9;
	int32_t accept_at = test->accept_at < 0 ? CONFIG_SD_RETRY_COUNT + 1 : test->accept_at;
	uint8_t expected_data;

	zassert_equal_ptr(dev, DEVICE_GET(sdio_speed_host));
	zassert_is_null(data);
	zassert_equal(cmd->opcode, 52U, "Expected CMD52");
	zassert_equal(cmd->response_type, SD_RSP_TYPE_R5 | SD_SPI_RSP_TYPE_R5);
	zassert_equal(cmd->timeout_ms, CONFIG_SD_CMD_TIMEOUT);
	if ((cmd->arg & BIT(31)) == 0U) {
		zassert_equal(cmd->arg, read_arg, "Unexpected CMD52 read encoding");
		fixture.reads++;
		if (test->read_error != 0) {
			return test->read_error;
		}
		cmd->response[0] = test->initial_reg;
		fixture.last_readback = test->initial_reg;
		return 0;
	}

	/* BSS bits are literal fixtures; other bits come from the latest readback. */
	expected_data = (fixture.last_readback & 0xF1U) | (test->want_write_data & 0x0EU);
	zassert_equal(cmd->arg, BIT(31) | BIT(27) | read_arg | expected_data,
		      "Unexpected CMD52 write/RAW/BSS encoding");
	fixture.writes++;
	if (test->write_error != 0 && fixture.writes == test->write_error_at) {
		return test->write_error;
	}
	fixture.card_accepted = accept_at > 0 && fixture.writes >= accept_at;
	fixture.last_readback = fixture.card_accepted ? test->accepted_reg : test->rejected_reg;
	cmd->response[0] = fixture.last_readback;
	return 0;
}

static int set_io(const struct device *dev, struct sdhc_io *io)
{
	zassert_equal_ptr(dev, DEVICE_GET(sdio_speed_host));
	zassert_true(fixture.card_accepted, "Host changed before CMD52 acceptance");
	fixture.requested_io = *io;
	return fixture.test->host_error;
}

void sdio_speed_run_case(const struct speed_case *test)
{
	struct sd_card card = {
		.sdhc = DEVICE_GET(sdio_speed_host),
		.bus_io = {
			.clock = 25000000U,
			.timing = test->voltage == SD_VOL_1_8_V ? SDHC_TIMING_SDR12 :
								       SDHC_TIMING_LEGACY,
			.signal_voltage = test->voltage,
		},
		.host_props = {
			.f_max = test->host_max,
			.host_caps = {
				.high_spd_support = true,
				.sdr50_support = true,
				.sdr104_support = true,
				.ddr50_support = true,
			},
		},
		.cccr_flags = test->caps,
	};
	uint32_t want_writes = test->want_writes < 0 ? CONFIG_SD_RETRY_COUNT + 1 :
						    test->want_writes;
	int32_t ret;

	fixture = (struct speed_fixture){.test = test};
	RESET_FAKE(fake_sdhc_request);
	RESET_FAKE(fake_sdhc_set_io);
	RESET_FAKE(fake_sdhc_execute_tuning);
	FFF_RESET_HISTORY();
	fake_sdhc_request_fake.custom_fake = request;
	fake_sdhc_set_io_fake.custom_fake = set_io;
	zassert_true(device_is_ready(card.sdhc));

	sdio_select_bus_speed(&card);
	ret = sdio_set_bus_speed(&card);

	zassert_equal(ret, test->want_ret, "Unexpected return code");
	zassert_equal(fixture.reads, 1U, "Must read initial CCCR speed once");
	zassert_equal(fixture.writes, want_writes, "Unexpected speed write count");
	zassert_equal(fake_sdhc_request_fake.call_count, 1U + want_writes);
	zassert_equal(fake_sdhc_set_io_fake.call_count, test->want_sets,
		      "Host changes require accepted card speed");
	zassert_equal(card.bus_io.clock, test->want_clock);
	zassert_equal(card.bus_io.timing, test->want_timing);
	zassert_equal(card.bus_io.signal_voltage, test->voltage);
	if (test->want_sets > 0U) {
		zassert_equal(fixture.requested_io.clock, test->want_clock);
		zassert_equal(fixture.requested_io.timing, test->want_timing);
		zassert_equal(fixture.requested_io.signal_voltage, test->voltage);
	}
}

struct sdio_init_fixture {
	const struct init_case *test;
	struct sd_card *card;
	uint32_t ocr_calls;
	uint32_t rca_calls;
	uint32_t select_calls;
	uint32_t read_index;
	uint32_t width_writes;
	uint32_t speed_writes;
	uint8_t speed_reg;
	struct sdhc_io tuning_io;
};

static struct sdio_init_fixture init_fixture;

static const uint32_t init_read_addresses[] = {
	0x00U, 0x01U, 0x08U, 0x13U, 0x14U, 0x15U,
	0x09U, 0x0AU, 0x0BU, 0x1000U, 0x07U, 0x13U,
};

static int init_request(const struct device *dev, struct sdhc_command *cmd,
			struct sdhc_data *data)
{
	uint32_t address;
	bool write;

	zassert_equal_ptr(dev, DEVICE_GET(sdio_speed_host));
	zassert_is_null(data);
	zassert_equal(cmd->timeout_ms, CONFIG_SD_CMD_TIMEOUT);
	switch (cmd->opcode) {
	case 5U:
		zassert_equal(cmd->response_type, SD_RSP_TYPE_R4 | SD_SPI_RSP_TYPE_R4);
		zassert_true(init_fixture.ocr_calls < 2U);
		zassert_equal(cmd->arg, init_fixture.ocr_calls == 0U ? 0U : 0x01300000U);
		init_fixture.ocr_calls++;
		cmd->response[0] = 0x91300000U;
		return 0;
	case 3U:
		zassert_equal(init_fixture.ocr_calls, 2U);
		zassert_equal(init_fixture.rca_calls++, 0U);
		zassert_equal(cmd->response_type, SD_RSP_TYPE_R6);
		zassert_equal(cmd->arg, 0U);
		cmd->response[0] = 0x00010000U;
		return 0;
	case 7U:
		zassert_equal(init_fixture.rca_calls, 1U);
		zassert_equal(init_fixture.select_calls++, 0U);
		zassert_equal(cmd->response_type, SD_RSP_TYPE_R1);
		zassert_equal(cmd->arg, 0x00010000U);
		cmd->response[0] = 0x00000900U;
		return 0;
	case 52U:
		zassert_equal(init_fixture.select_calls, 1U);
		zassert_equal(cmd->response_type, SD_RSP_TYPE_R5 | SD_SPI_RSP_TYPE_R5);
		break;
	default:
		zassert_unreachable("Unexpected initialization command %u", cmd->opcode);
		return -EINVAL;
	}

	address = (cmd->arg >> 9) & 0x1FFFFU;
	write = (cmd->arg & BIT(31)) != 0U;
	if (write) {
		uint8_t value;

		if (address == 0x07U) {
			zassert_equal(init_fixture.read_index, 11U);
			zassert_equal(init_fixture.width_writes++, 0U);
			value = 0x02U;
		} else {
			zassert_equal(address, 0x13U);
			zassert_equal(init_fixture.read_index, ARRAY_SIZE(init_read_addresses));
			zassert_equal(init_fixture.width_writes, 1U);
			init_fixture.speed_writes++;
			value = init_fixture.test->speed_write;
		}
		zassert_equal(cmd->arg, BIT(31) | BIT(27) | (address << 9) | value);
		if (address == 0x13U) {
			init_fixture.speed_reg = init_fixture.test->accept ? value : 0x01U;
			value = init_fixture.speed_reg;
		}
		cmd->response[0] = value;
		return 0;
	}

	zassert_equal(cmd->arg, address << 9);
	zassert_true(init_fixture.read_index < ARRAY_SIZE(init_read_addresses));
	zassert_equal(address, init_read_addresses[init_fixture.read_index++]);
	switch (address) {
	case 0x00U:
	case 0x01U:
		cmd->response[0] = 0x03U;
		break;
	case 0x13U:
		cmd->response[0] = init_fixture.speed_reg;
		break;
	case 0x14U:
		cmd->response[0] = init_fixture.test->uhs_reg;
		break;
	case 0x0AU:
		cmd->response[0] = 0x10U;
		break;
	case 0x1000U:
		cmd->response[0] = 0xFFU;
		break;
	default:
		cmd->response[0] = 0U;
		break;
	}
	return 0;
}

static int init_set_io(const struct device *dev, struct sdhc_io *io)
{
	zassert_equal_ptr(dev, DEVICE_GET(sdio_speed_host));
	zassert_equal(io->signal_voltage, SD_VOL_1_8_V);
	zassert_equal(io->bus_width, SDHC_BUS_WIDTH4BIT);
	if (fake_sdhc_set_io_fake.call_count == 1U) {
		zassert_equal(init_fixture.width_writes, 1U);
		zassert_equal(init_fixture.speed_writes, 0U);
		zassert_equal(io->clock, 25000000U);
		zassert_equal(io->timing, SDHC_TIMING_SDR12);
	} else {
		zassert_true(init_fixture.test->accept);
		zassert_equal(init_fixture.speed_writes, 1U);
		zassert_equal(io->clock, init_fixture.test->want_clock);
		zassert_equal(io->timing, init_fixture.test->want_timing);
	}
	return 0;
}

static int init_tune(const struct device *dev)
{
	zassert_equal_ptr(dev, DEVICE_GET(sdio_speed_host));
	init_fixture.tuning_io = init_fixture.card->bus_io;
	return init_fixture.test->tuning_error;
}

void sdio_speed_run_init_case(const struct init_case *test)
{
	struct sd_card card = {
		.sdhc = DEVICE_GET(sdio_speed_host),
		.bus_io = {
			.clock = 25000000U,
			.timing = SDHC_TIMING_SDR12,
			.signal_voltage = SD_VOL_1_8_V,
			.bus_width = SDHC_BUS_WIDTH1BIT,
		},
		.card_voltage = SD_VOL_1_8_V,
		.host_props = {
			.f_max = 208000000U,
			.bus_4_bit_support = true,
			.host_caps = {
				.vol_180_support = true,
				.high_spd_support = true,
				.sdr50_support = true,
				.sdr104_support = true,
			},
		},
	};
	uint32_t want_writes = test->accept ? 1U : CONFIG_SD_RETRY_COUNT + 1U;
	int32_t ret;

	init_fixture = (struct sdio_init_fixture){
		.test = test,
		.card = &card,
		.speed_reg = 0x01U,
	};
	RESET_FAKE(fake_sdhc_request);
	RESET_FAKE(fake_sdhc_set_io);
	RESET_FAKE(fake_sdhc_execute_tuning);
	FFF_RESET_HISTORY();
	fake_sdhc_request_fake.custom_fake = init_request;
	fake_sdhc_set_io_fake.custom_fake = init_set_io;
	fake_sdhc_execute_tuning_fake.custom_fake = init_tune;
	zassert_true(device_is_ready(card.sdhc));

	ret = sdio_card_init(&card);
	printk("SDIO init: uhs=0x%02x accept=%u requested=%u raw=0x%02x "
	       "clock=%u timing=%u width=%u sets=%u tunes=%u ret=%d\n",
	       test->uhs_reg, test->accept, card.card_speed, init_fixture.speed_reg,
	       card.bus_io.clock, card.bus_io.timing, card.bus_io.bus_width,
	       fake_sdhc_set_io_fake.call_count, fake_sdhc_execute_tuning_fake.call_count, ret);

	zassert_equal(ret, test->want_ret);
	zassert_equal(init_fixture.ocr_calls, 2U);
	zassert_equal(init_fixture.rca_calls, 1U);
	zassert_equal(init_fixture.select_calls, 1U);
	zassert_equal(init_fixture.read_index, ARRAY_SIZE(init_read_addresses));
	zassert_equal(init_fixture.width_writes, 1U);
	zassert_equal(init_fixture.speed_writes, want_writes);
	zassert_equal(fake_sdhc_request_fake.call_count, 17U + want_writes);
	zassert_equal(fake_sdhc_set_io_fake.call_count, test->accept ? 2U : 1U);
	zassert_equal(fake_sdhc_execute_tuning_fake.call_count, test->want_tunes);
	zassert_equal(card.type, CARD_SDIO);
	zassert_equal(card.num_io, 1U);
	zassert_equal(card.relative_addr, 1U);
	zassert_not_equal(card.flags & SD_1800MV_FLAG, 0U);
	zassert_equal_ptr(card.func0.card, &card);
	zassert_equal(card.bus_io.clock, test->want_clock);
	zassert_equal(card.bus_io.timing, test->want_timing);
	zassert_equal(card.bus_io.signal_voltage, SD_VOL_1_8_V);
	zassert_equal(card.bus_io.bus_width, SDHC_BUS_WIDTH4BIT);
	if (test->want_tunes > 0U) {
		zassert_equal(init_fixture.tuning_io.clock, test->want_clock);
		zassert_equal(init_fixture.tuning_io.timing, test->want_timing);
	}
}
