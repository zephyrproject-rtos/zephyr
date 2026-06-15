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

/* Verify fault injection on CMD17 returns -EIO and clears correctly. */
ZTEST(sdhc_faults, test_inject_cmd17_error)
{
	struct sdhc_command cmd = {0};

	sdhc_emul_set_fault(dev, SD_READ_SINGLE_BLOCK);

	cmd.opcode = SD_READ_SINGLE_BLOCK;

	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_equal(ret, -EIO, "CMD17 should fail due to fault injection");

	sdhc_emul_clear_fault(dev);

	cmd.opcode = SD_READ_SINGLE_BLOCK;
	ret = sdhc_request(dev, &cmd, NULL);
	zassert_not_equal(ret, -EIO, "CMD17 fault should be cleared");
}

/* Verify I/O after card removal returns -ENODEV. */
ZTEST(sdhc_faults, test_card_removal_mid_io)
{
	struct sdhc_command cmd = {0};

	sdhc_emul_set_card_present(dev, false);

	cmd.opcode = SD_READ_MULTIPLE_BLOCK;

	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_equal(ret, -ENODEV, "I/O should fail with -ENODEV after card removal");

	sdhc_emul_set_card_present(dev, true);
}

/* Verify CMD17 with NULL data buffer returns -EINVAL. */
ZTEST(sdhc_faults, test_null_data_buffer)
{
	(void)sdhc_init_to_transfer(dev);
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};

	cmd.opcode = SD_READ_SINGLE_BLOCK;
	data.data = NULL;

	int ret = sdhc_request(dev, &cmd, &data);

	zassert_equal(ret, -EINVAL, "CMD17 with NULL buffer should return -EINVAL");
}

/* Verify CMD17 with out-of-range block address returns -EINVAL. */
ZTEST(sdhc_faults, test_out_of_range_block)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};
	uint8_t buf[512];

	cmd.opcode = SD_READ_SINGLE_BLOCK;
	data.block_addr = UINT32_MAX;
	data.data = buf;

	int ret = sdhc_request(dev, &cmd, &data);

	zassert_equal(ret, -EINVAL, "CMD17 with out of bounds block should return -EINVAL");
}

/* Verify CMD8 echo reflects the supplied check pattern. */
ZTEST(sdhc_faults, test_wrong_voltage_cmd8)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SD_SEND_IF_COND;
	cmd.arg = 0x00000000;

	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD8 failed");
	zassert_equal(cmd.response[0] & 0xFF, 0x00, "CMD8 echo pattern mismatch");
}

/* Verify consecutive GO_IDLE_STATE commands succeed. */
ZTEST(sdhc_faults, test_double_reset)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SD_GO_IDLE_STATE;
	zassert_ok(sdhc_request(dev, &cmd, NULL), "First reset failed");
	zassert_ok(sdhc_request(dev, &cmd, NULL), "Second reset failed");
}

/* Verify fault injection on CMD24 returns -EIO and leaves state intact. */
ZTEST(sdhc_faults, test_inject_cmd24_error)
{
	uint32_t rca = sdhc_init_to_transfer(dev);
	struct sdhc_command cmd = {0};

	sdhc_emul_set_fault(dev, SD_WRITE_SINGLE_BLOCK);

	cmd.opcode = SD_WRITE_SINGLE_BLOCK;
	struct sdhc_data data = {0};
	uint8_t wbuf[512] = {0};

	data.block_addr = 0;
	data.block_size = 512;
	data.blocks = 1;
	data.data = wbuf;

	int ret = sdhc_request(dev, &cmd, &data);

	zassert_equal(ret, -EIO, "CMD24 should fail due to fault injection");

	int state = sdhc_get_card_state(dev, rca);

	zassert_true(state >= 0, "CMD13 failed");
	zassert_equal(state, 4, "Card state should stay TRANSFER after failed CMD24");

	sdhc_emul_clear_fault(dev);
}

/* Verify fault on ACMD41 keeps the card in IDLE state. */
ZTEST(sdhc_faults, test_inject_acmd41_blocks_init)
{
	struct sdhc_command cmd = {0};

	/* Reset to IDLE */
	cmd.opcode = SD_GO_IDLE_STATE;
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD0 failed");

	/* Inject fault on ACMD41 */
	sdhc_emul_set_fault(dev, SD_APP_SEND_OP_COND);

	cmd.opcode = SD_APP_CMD;
	cmd.arg = 0;
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD55 failed");

	cmd.opcode = SD_APP_SEND_OP_COND;
	cmd.arg = 0x40FF8000;
	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_equal(ret, -EIO, "ACMD41 should fail due to fault injection");

	/* Card must stay in IDLE (not advance to READY) */
	int state = sdhc_get_card_state(dev, 0);

	zassert_true(state >= 0, "CMD13 failed");
	zassert_equal(state, 0, "Card should stay IDLE after failed ACMD41");

	sdhc_emul_clear_fault(dev);
}

/* Verify fault on CMD8 keeps the card in IDLE state. */
ZTEST(sdhc_faults, test_inject_cmd8_blocks_init)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SD_GO_IDLE_STATE;
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD0 failed");

	sdhc_emul_set_fault(dev, SD_SEND_IF_COND);

	cmd.opcode = SD_SEND_IF_COND;
	cmd.arg = 0x000001AA;

	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_equal(ret, -EIO, "CMD8 should fail due to fault injection");

	/* Card must stay in IDLE */
	int state = sdhc_get_card_state(dev, 0);

	zassert_true(state >= 0, "CMD13 failed");
	zassert_equal(state, 0, "Card should stay IDLE after failed CMD8");

	sdhc_emul_clear_fault(dev);
}
