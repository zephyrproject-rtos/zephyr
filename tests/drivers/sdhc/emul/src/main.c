/*
 * Copyright 2026 Alif Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/sdhc/sdhc_emul.h>
#include <zephyr/sd/sd_spec.h>
#include <string.h>

static const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(sdhc_sd));

static int card_state(const struct device *dev, uint32_t rca)
{
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = SD_SEND_STATUS;
	cmd.arg = rca << 16;
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = 200;
	ret = sdhc_request(dev, &cmd, NULL);
	if (ret != 0) {
		return ret;
	}
	return (cmd.response[0] >> SD_R1_CURRENT_STATE_OFFSET) & 0xF;
}

/* CMD0 through CMD7 so later tests start in TRANSFER. SD cards only. */
static void sd_to_transfer(const struct device *card_dev)
{
	struct sdhc_command cmd = {0};
	uint32_t rca;

	cmd.opcode = SD_GO_IDLE_STATE;
	zassert_ok(sdhc_request(card_dev, &cmd, NULL), "CMD0 failed");

	cmd.opcode = SD_APP_CMD;
	zassert_ok(sdhc_request(card_dev, &cmd, NULL), "CMD55 failed");
	cmd.opcode = SD_APP_SEND_OP_COND;
	cmd.arg = SD_OCR_HOST_CAP_FLAG;
	zassert_ok(sdhc_request(card_dev, &cmd, NULL), "ACMD41 failed");

	cmd.opcode = SD_ALL_SEND_CID;
	cmd.arg = 0;
	zassert_ok(sdhc_request(card_dev, &cmd, NULL), "CMD2 failed");

	cmd.opcode = SD_SEND_RELATIVE_ADDR;
	cmd.arg = 0;
	zassert_ok(sdhc_request(card_dev, &cmd, NULL), "CMD3 failed");
	rca = cmd.response[0] >> 16;

	cmd.opcode = SD_SELECT_CARD;
	cmd.arg = rca << 16;
	zassert_ok(sdhc_request(card_dev, &cmd, NULL), "CMD7 failed");
}

ZTEST(sdhc_init, test_card_present_toggle)
{
	zassert_equal(sdhc_card_present(dev), 1, "Card not present on init");
	sdhc_emul_set_card_present(dev, false);
	zassert_equal(sdhc_card_present(dev), 0, "Card still present after removal");
	sdhc_emul_set_card_present(dev, true);
	zassert_equal(sdhc_card_present(dev), 1, "Card not present after reinsertion");
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
	state = card_state(dev, 0);
	zassert_equal(state, 0, "Not IDLE after CMD0");

	/* CMD8 -> IDLE */
	cmd.opcode = SD_SEND_IF_COND;
	cmd.arg = SD_IF_COND_VHS_3V3 | SD_IF_COND_CHECK;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD8 failed");
	zassert_equal(cmd.response[0] & SD_IF_COND_CHECK, SD_IF_COND_CHECK,
			  "CMD8 bad echo pattern");
	state = card_state(dev, 0);
	zassert_equal(state, 0, "Not IDLE after CMD8");

	/* ACMD41 -> READY */
	cmd.opcode = SD_APP_CMD;
	cmd.arg = 0;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD55 failed");

	cmd.opcode = SD_APP_SEND_OP_COND;
	cmd.arg = SD_OCR_HOST_CAP_FLAG |
		  SD_OCR_VDD27_28FLAG | SD_OCR_VDD28_29FLAG |
		  SD_OCR_VDD29_30FLAG | SD_OCR_VDD30_31FLAG |
		  SD_OCR_VDD31_32FLAG | SD_OCR_VDD32_33FLAG |
		  SD_OCR_VDD33_34FLAG | SD_OCR_VDD34_35FLAG |
		  SD_OCR_VDD35_36FLAG;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "ACMD41 failed");
	zassert_true(cmd.response[0] & SD_OCR_PWR_BUSY_FLAG,
		     "Card not ready after ACMD41");
			 state = card_state(dev, 0);
	zassert_equal(state, 1, "Not READY after ACMD41");

	/* CMD2 -> IDENT */
	cmd.opcode = SD_ALL_SEND_CID;
	cmd.arg = 0;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD2 failed");
	state = card_state(dev, 0);
	zassert_equal(state, 2, "Not IDENT after CMD2");

	/* CMD3 -> STANDBY */
	cmd.opcode = SD_SEND_RELATIVE_ADDR;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD3 failed");
	uint32_t rca = cmd.response[0] >> 16;

	state = card_state(dev, rca);
	zassert_equal(state, 3, "Not STANDBY after CMD3");

	/* CMD7 -> TRANSFER */
	cmd.opcode = SD_SELECT_CARD;
	cmd.arg = rca << 16;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD7 failed");
	state = card_state(dev, rca);
	zassert_equal(state, 4, "Not TRANSFER after CMD7");
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
	state = card_state(emmc_dev, 0);
	zassert_equal(state, 0, "Not IDLE after CMD0");

	cmd.opcode = MMC_SEND_OP_COND;
	cmd.arg = SD_OCR_HOST_CAP_FLAG |
		  SD_OCR_VDD27_28FLAG | SD_OCR_VDD28_29FLAG |
		  SD_OCR_VDD29_30FLAG | SD_OCR_VDD30_31FLAG |
		  SD_OCR_VDD31_32FLAG | SD_OCR_VDD32_33FLAG |
		  SD_OCR_VDD33_34FLAG | SD_OCR_VDD34_35FLAG |
		  SD_OCR_VDD35_36FLAG;
	ret = sdhc_request(emmc_dev, &cmd, NULL);
	zassert_ok(ret, "CMD1 failed");
	zassert_true(cmd.response[0] & SD_OCR_PWR_BUSY_FLAG,
		     "eMMC not ready after CMD1");
	state = card_state(emmc_dev, 0);
	zassert_equal(state, 1, "Not READY after CMD1");

	cmd.opcode = SD_ALL_SEND_CID;
	ret = sdhc_request(emmc_dev, &cmd, NULL);
	zassert_ok(ret, "CMD2 failed");
	state = card_state(emmc_dev, 0);
	zassert_equal(state, 2, "Not IDENT after CMD2");

	cmd.opcode = SD_SEND_RELATIVE_ADDR;
	ret = sdhc_request(emmc_dev, &cmd, NULL);
	zassert_ok(ret, "CMD3 failed");
	uint32_t rca = cmd.response[0] >> 16;

	state = card_state(emmc_dev, rca);
	zassert_equal(state, 3, "Not STANDBY after CMD3");

	cmd.opcode = SD_SELECT_CARD;
	cmd.arg = rca << 16;
	ret = sdhc_request(emmc_dev, &cmd, NULL);
	zassert_ok(ret, "CMD7 failed");
	state = card_state(emmc_dev, rca);
	zassert_equal(state, 4, "Not TRANSFER after CMD7");

	/* CMD8 EXT_CSD Read */
	cmd.opcode = MMC_SEND_EXT_CSD;
	cmd.arg = 0;
	struct sdhc_data data = {0};
	uint8_t ext_csd[512] = {0};

	data.data = ext_csd;
	ret = sdhc_request(emmc_dev, &cmd, &data);
	zassert_ok(ret, "CMD8 EXT_CSD read failed");
	zassert_equal(ext_csd[MMC_EXT_CSD_REV], MMC_5_1,
		      "EXT_CSD_REV mismatch (expected eMMC 5.1)");
}

/* Poke backing storage, then CMD17 must return the same bytes. */
ZTEST(sdhc_init, test_storage_and_block_count_accessors)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};
	uint8_t rbuf[512];
	uint32_t count = 0;
	uint8_t *storage = sdhc_emul_get_storage(dev);

	zassert_not_null(storage, "Storage pointer should not be NULL");
	zassert_ok(sdhc_emul_get_block_count(dev, &count));
	zassert_equal(count, 1024, "Unexpected block count for sdhc_sd");

	memset(storage, 0x3C, 512);
	sd_to_transfer(dev);

	cmd.opcode = SD_READ_SINGLE_BLOCK;
	cmd.arg = 0;
	data.block_addr = 0;
	data.block_size = 512;
	data.blocks = 1;
	data.data = rbuf;
	zassert_ok(sdhc_request(dev, &cmd, &data), "CMD17 failed");
	zassert_mem_equal(rbuf, storage, 512, "CMD17 did not read backing storage");
}

ZTEST_SUITE(sdhc_init, NULL, NULL, NULL, NULL, NULL);
