/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_common.h"
#include <zephyr/ztest.h>
#include <zephyr/sd/sd_spec.h>

uint32_t sdhc_init_to_transfer(const struct device *dev)
{
	struct sdhc_host_props props = {0};
	struct sdhc_command cmd = {0};
	int ret;

	zassert_ok(sdhc_get_host_props(dev, &props), "get_host_props failed");

	bool is_emmc = props.host_caps.bus_8_bit_support != 0;

	/* CMD0 GO_IDLE_STATE */
	cmd.opcode = SD_GO_IDLE_STATE;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD0 failed");

	if (is_emmc) {
		/* CMD1 SEND_OP_COND */
		cmd.opcode = MMC_SEND_OP_COND;
		cmd.arg = 0x40FF8000;
		ret = sdhc_request(dev, &cmd, NULL);
		zassert_ok(ret, "CMD1 failed");
		zassert_true(cmd.response[0] & 0x80000000,
			     "eMMC not ready after CMD1");
	} else {
		/* CMD8 SEND_IF_COND */
		cmd.opcode = SD_SEND_IF_COND;
		cmd.arg = 0x000001AA;
		ret = sdhc_request(dev, &cmd, NULL);
		zassert_ok(ret, "CMD8 failed");
		zassert_equal(cmd.response[0] & 0xFF, 0xAA,
			      "CMD8 bad echo pattern");

		/* CMD55 + ACMD41 SD_SEND_OP_COND */
		cmd.opcode = SD_APP_CMD;
		cmd.arg = 0;
		ret = sdhc_request(dev, &cmd, NULL);
		zassert_ok(ret, "CMD55 failed");

		cmd.opcode = SD_APP_SEND_OP_COND;
		cmd.arg = 0x40FF8000;
		ret = sdhc_request(dev, &cmd, NULL);
		zassert_ok(ret, "ACMD41 failed");
		zassert_true(cmd.response[0] & 0x80000000,
			     "Card not ready after ACMD41");
	}

	/* CMD2 ALL_SEND_CID */
	cmd.opcode = SD_ALL_SEND_CID;
	cmd.arg = 0;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD2 failed");

	/* CMD3 SEND_RELATIVE_ADDR */
	cmd.opcode = SD_SEND_RELATIVE_ADDR;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD3 failed");
	uint32_t rca = cmd.response[0] >> 16;

	/* CMD7 SELECT_CARD */
	cmd.opcode = SD_SELECT_CARD;
	cmd.arg = rca << 16;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_ok(ret, "CMD7 failed");

	return rca;
}

int sdhc_get_card_state(const struct device *dev, uint32_t rca)
{
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = SD_SEND_STATUS;
	cmd.arg = rca << 16;
	ret = sdhc_request(dev, &cmd, NULL);
	if (ret != 0) {
		return ret;
	}
	return (cmd.response[0] >> 9) & 0xF;
}

int sdhc_acmd(const struct device *dev, uint32_t rca,
	      uint8_t opcode, uint32_t arg,
	      struct sdhc_command *cmd_out,
	      struct sdhc_data *data)
{
	struct sdhc_command cmd = {0};
	int ret;

	cmd.opcode = SD_APP_CMD;
	cmd.arg = rca << 16;
	ret = sdhc_request(dev, &cmd, NULL);
	if (ret != 0) {
		return ret;
	}

	cmd.opcode = opcode;
	cmd.arg = arg;
	ret = sdhc_request(dev, &cmd, data);
	if (cmd_out != NULL) {
		*cmd_out = cmd;
	}
	return ret;
}

int sdhc_write_blocks(const struct device *dev, uint32_t block_addr,
		      uint8_t *buf, uint32_t n_blocks, size_t *bytes_xfered)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};

	cmd.opcode = (n_blocks == 1) ? SD_WRITE_SINGLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK;
	cmd.arg = block_addr;
	data.block_addr = block_addr;
	data.block_size = 512;
	data.blocks = n_blocks;
	data.data = buf;

	int ret = sdhc_request(dev, &cmd, &data);

	if (ret == 0 && bytes_xfered != NULL) {
		*bytes_xfered = data.bytes_xfered;
	}
	return ret;
}

int sdhc_read_blocks(const struct device *dev, uint32_t block_addr,
		     uint8_t *buf, uint32_t n_blocks, size_t *bytes_xfered)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};

	cmd.opcode = (n_blocks == 1) ? SD_READ_SINGLE_BLOCK : SD_READ_MULTIPLE_BLOCK;
	cmd.arg = block_addr;
	data.block_addr = block_addr;
	data.block_size = 512;
	data.blocks = n_blocks;
	data.data = buf;

	int ret = sdhc_request(dev, &cmd, &data);

	if (ret == 0 && bytes_xfered != NULL) {
		*bytes_xfered = data.bytes_xfered;
	}
	return ret;
}
