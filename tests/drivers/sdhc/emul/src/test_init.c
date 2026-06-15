/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/sdhc/sdhc_emul.h>
#include <zephyr/sd/sd_spec.h>
#include "test_common.h"

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(sdhc_sd));

/* Verify SD host properties report expected capabilities. */
ZTEST(sdhc_init, test_host_props_sd)
{
	struct sdhc_host_props props;

	zassert_true(device_is_ready(dev), "SDHC emulator not ready");
	zassert_ok(sdhc_get_host_props(dev, &props), "sdhc_get_host_props failed");
	zassert_true(props.f_max >= 25000000, "f_max too low for SD");
	zassert_equal(props.host_caps.high_spd_support, 1, "High speed not supported");
	zassert_equal(props.host_caps.bus_8_bit_support, 0, "8-bit bus supported on SD");
}

/* Verify eMMC host properties report expected capabilities. */
ZTEST(sdhc_init, test_host_props_emmc)
{
	const struct device *emmc_dev = DEVICE_DT_GET(DT_ALIAS(sdhc_emmc));
	struct sdhc_host_props props;

	zassert_true(device_is_ready(emmc_dev), "eMMC emulator not ready");
	zassert_ok(sdhc_get_host_props(emmc_dev, &props), "sdhc_get_host_props failed");
	zassert_equal(props.host_caps.bus_8_bit_support, 1, "8-bit bus not supported on eMMC");
	zassert_true(props.f_max >= 200000000, "f_max too low for eMMC");
}

/* Verify card presence can be toggled via the emulator accessor. */
ZTEST(sdhc_init, test_card_present_toggle)
{
	zassert_equal(sdhc_card_present(dev), 1, "Card not present on init");

	sdhc_emul_set_card_present(dev, false);
	zassert_equal(sdhc_card_present(dev), 0, "Card still present after removal");

	sdhc_emul_set_card_present(dev, true);
	zassert_equal(sdhc_card_present(dev), 1, "Card not present after reinsertion");
}

/* Verify hardware reset does not erase emulator storage contents. */
ZTEST(sdhc_init, test_reset_preserves_storage)
{
	uint8_t *storage = sdhc_emul_get_storage(dev);

	zassert_not_null(storage, "Storage pointer is NULL");

	storage[0] = 0xA5;
	zassert_ok(sdhc_hw_reset(dev), "sdhc_hw_reset failed");
	zassert_equal(storage[0], 0xA5, "Storage cleared on reset");
}

/* Verify full SD init sequence and state transitions via CMD13. */
ZTEST(sdhc_init, test_sd_init_sequence)
{
	struct sdhc_command cmd = {0};
	int ret;
	int state;

	/* CMD0 -> IDLE */
	cmd.opcode = SD_GO_IDLE_STATE;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD0 failed");
	state = sdhc_get_card_state(dev, 0);
	zassert_equal(state, 0, "Not IDLE after CMD0");

	/* CMD8 -> IDLE */
	cmd.opcode = SD_SEND_IF_COND;
	cmd.arg = 0x000001AA;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD8 failed");
	zassert_equal(cmd.response[0] & 0xFF, 0xAA, "CMD8 bad echo pattern");
	state = sdhc_get_card_state(dev, 0);
	zassert_equal(state, 0, "Not IDLE after CMD8");

	/* ACMD41 -> READY */
	cmd.opcode = SD_APP_CMD;
	cmd.arg = 0;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD55 failed");

	cmd.opcode = SD_APP_SEND_OP_COND;
	cmd.arg = 0x40FF8000;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "ACMD41 failed");
	zassert_true(cmd.response[0] & 0x80000000, "Card not ready after ACMD41");
	state = sdhc_get_card_state(dev, 0);
	zassert_equal(state, 1, "Not READY after ACMD41");

	/* CMD2 -> IDENT */
	cmd.opcode = SD_ALL_SEND_CID;
	cmd.arg = 0;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD2 failed");
	state = sdhc_get_card_state(dev, 0);
	zassert_equal(state, 2, "Not IDENT after CMD2");

	/* CMD3 -> STANDBY */
	cmd.opcode = SD_SEND_RELATIVE_ADDR;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD3 failed");
	uint32_t rca = cmd.response[0] >> 16;

	state = sdhc_get_card_state(dev, rca);
	zassert_equal(state, 3, "Not STANDBY after CMD3");

	/* CMD7 -> TRANSFER */
	cmd.opcode = SD_SELECT_CARD;
	cmd.arg = rca << 16;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD7 failed");
	state = sdhc_get_card_state(dev, rca);
	zassert_equal(state, 4, "Not TRANSFER after CMD7");
}

/* Verify CMD17 read is rejected when the card is in IDLE state. */
ZTEST(sdhc_init, test_illegal_state_cmd17_in_idle)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};

	cmd.opcode = SD_GO_IDLE_STATE;
	sdhc_request(dev, &cmd, NULL);

	cmd.opcode = SD_READ_SINGLE_BLOCK;
	cmd.arg = 0;

	int ret = sdhc_request(dev, &cmd, &data);

	zassert_not_equal(ret, 0, "CMD17 should fail in IDLE state");
}

/* Verify full eMMC init sequence and state transitions via CMD13. */
ZTEST(sdhc_init, test_emmc_init_sequence)
{
	const struct device *emmc_dev = DEVICE_DT_GET(DT_ALIAS(sdhc_emmc));
	struct sdhc_command cmd = {0};
	int ret;
	int state;

	cmd.opcode = SD_GO_IDLE_STATE;
	ret = sdhc_request(emmc_dev, &cmd, NULL);
	zassert_ok(ret, "CMD0 failed");
	state = sdhc_get_card_state(emmc_dev, 0);
	zassert_equal(state, 0, "Not IDLE after CMD0");

	cmd.opcode = MMC_SEND_OP_COND;
	cmd.arg = 0x40FF8000;
	ret = sdhc_request(emmc_dev, &cmd, NULL);
	zassert_ok(ret, "CMD1 failed");
	zassert_true(cmd.response[0] & 0x80000000, "eMMC not ready after CMD1");
	state = sdhc_get_card_state(emmc_dev, 0);
	zassert_equal(state, 1, "Not READY after CMD1");

	cmd.opcode = SD_ALL_SEND_CID;
	ret = sdhc_request(emmc_dev, &cmd, NULL);
	zassert_ok(ret, "CMD2 failed");
	state = sdhc_get_card_state(emmc_dev, 0);
	zassert_equal(state, 2, "Not IDENT after CMD2");

	cmd.opcode = SD_SEND_RELATIVE_ADDR;
	ret = sdhc_request(emmc_dev, &cmd, NULL);
	zassert_ok(ret, "CMD3 failed");
	uint32_t rca = cmd.response[0] >> 16;

	state = sdhc_get_card_state(emmc_dev, rca);
	zassert_equal(state, 3, "Not STANDBY after CMD3");

	cmd.opcode = SD_SELECT_CARD;
	cmd.arg = rca << 16;
	ret = sdhc_request(emmc_dev, &cmd, NULL);
	zassert_ok(ret, "CMD7 failed");
	state = sdhc_get_card_state(emmc_dev, rca);
	zassert_equal(state, 4, "Not TRANSFER after CMD7");

	/* CMD8 EXT_CSD Read */
	cmd.opcode = MMC_SEND_EXT_CSD;
	cmd.arg = 0;
	struct sdhc_data data = {0};
	uint8_t ext_csd[512] = {0};

	data.data = ext_csd;
	ret = sdhc_request(emmc_dev, &cmd, &data);
	zassert_ok(ret, "CMD8 EXT_CSD read failed");
	zassert_equal(ext_csd[EXT_CSD_STRUCTURE], MMC_5_1,
		      "EXT_CSD_REV mismatch (expected eMMC 5.1)");
}

/* Verify CSD capacity field matches the devicetree block count. */
ZTEST(sdhc_init, test_csd_capacity_matches_config)
{
	uint32_t rca = sdhc_init_to_transfer(dev);
	struct sdhc_command cmd = {0};

	cmd.opcode = SD_SEND_CSD;
	cmd.arg = rca << 16;

	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_ok(ret, "CMD9 failed");

	uint32_t c_size = ((cmd.response[1] & 0x3F) << 16) |
			  ((cmd.response[2] & 0xFFFF0000U) >> 16);
	uint32_t block_count = (c_size + 1) * 1024;
	uint32_t expected_blocks = sdhc_emul_get_block_count(dev);

	zassert_equal(block_count, expected_blocks, "CSD capacity mismatch");
}

/* Verify ACMD6 and sdhc_set_io accept 4-bit bus width. */
ZTEST(sdhc_init, test_set_bus_width_4bit)
{
	uint32_t rca = sdhc_init_to_transfer(dev);
	struct sdhc_command cmd;

	zassert_ok(sdhc_acmd(dev, rca, 6, 2, &cmd, NULL), "ACMD6 failed");

	struct sdhc_io ios = {
		.bus_width = SDHC_BUS_WIDTH4BIT,
	};
	zassert_ok(sdhc_set_io(dev, &ios), "sdhc_set_io 4-bit failed");
}

/* Verify ACMD51 returns a valid SCR structure. */
ZTEST(sdhc_init, test_acmd51_scr_read)
{
	uint32_t rca = sdhc_init_to_transfer(dev);
	struct sdhc_command cmd;
	struct sdhc_data data = {0};
	uint8_t scr[8] = {0};

	data.data = scr;
	data.blocks = 1;
	data.block_size = 8;

	zassert_ok(sdhc_acmd(dev, rca, 51, 0, &cmd, &data), "ACMD51 failed");
	/* Byte 0: lower nibble is SD_SPEC, must be v2.0 or later */
	zassert_true((scr[0] & 0x0F) >= 2, "SCR SD_SPEC too low");
	/* Byte 1: bit 2 indicates 4-bit bus support */
	zassert_true(scr[1] & 0x04, "4-bit bus not indicated in SCR");
}
