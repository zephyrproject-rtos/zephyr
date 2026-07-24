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

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(sdhc_emmc));

void *sdhc_emmc_setup(void)
{
	zassert_true(device_is_ready(dev), "eMMC emulator not ready");
	sdhc_init_to_transfer(dev);
	return NULL;
}

/* Verify EXT_CSD read returns SEC_COUNT matching devicetree capacity. */
ZTEST(sdhc_emmc, test_ext_csd_read)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};
	uint8_t ext_csd[512] = {0};

	cmd.opcode = MMC_SEND_EXT_CSD;
	data.data = ext_csd;

	int ret = sdhc_request(dev, &cmd, &data);

	zassert_ok(ret, "CMD8 failed");
	zassert_equal(data.bytes_xfered, 512, "bytes_xfered mismatch for EXT_CSD");

	uint32_t cap = sdhc_emul_get_block_count(dev);
	uint32_t sec_count = (ext_csd[EXT_CSD_SEC_COUNT + 3] << 24) |
			     (ext_csd[EXT_CSD_SEC_COUNT + 2] << 16) |
			     (ext_csd[EXT_CSD_SEC_COUNT + 1] << 8) |
			     ext_csd[EXT_CSD_SEC_COUNT + 0];
	zassert_equal(sec_count, cap, "SEC_COUNT mismatch");
}

/* Verify CMD6 SWITCH can set HS200 timing mode in EXT_CSD. */
ZTEST(sdhc_emmc, test_switch_hs200)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SD_SWITCH;
	cmd.arg = (3 << 24) | (EXT_CSD_HS_TIMING << 16) | (0x02 << 8);
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD6 SWITCH failed");

	struct sdhc_data data = {0};
	uint8_t ext_csd[512] = {0};

	cmd.opcode = MMC_SEND_EXT_CSD;
	cmd.arg = 0;
	data.data = ext_csd;
	sdhc_request(dev, &cmd, &data);
	zassert_equal(ext_csd[EXT_CSD_HS_TIMING], 0x02, "HS_TIMING not updated to HS200");
}

/* Verify CMD6 SWITCH can set 8-bit bus width in EXT_CSD. */
ZTEST(sdhc_emmc, test_switch_bus_width_8bit)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SD_SWITCH;
	cmd.arg = (3 << 24) | (EXT_CSD_BUS_WIDTH << 16) | (0x02 << 8);
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD6 SWITCH failed");

	struct sdhc_data data = {0};
	uint8_t ext_csd[512] = {0};

	cmd.opcode = MMC_SEND_EXT_CSD;
	cmd.arg = 0;
	data.data = ext_csd;
	sdhc_request(dev, &cmd, &data);
	zassert_equal(ext_csd[EXT_CSD_BUS_WIDTH], 0x02, "BUS_WIDTH not updated to 8-bit");
}

/* Verify reliable write prefix (CMD23 bit 31) is accepted. */
ZTEST(sdhc_emmc, test_reliable_write)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data sdhc_data = {0};
	uint8_t wbuf[512] = {0};

	cmd.opcode = SD_SET_BLOCK_COUNT;
	cmd.arg = (1U << 31) | 1;
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD23 failed");

	cmd.opcode = SD_WRITE_MULTIPLE_BLOCK;
	cmd.arg = 0;
	sdhc_data.block_addr = 0;
	sdhc_data.block_size = 512;
	sdhc_data.blocks = 1;
	sdhc_data.data = wbuf;
	zassert_ok(sdhc_request(dev, &cmd, &sdhc_data), "CMD25 failed");
	zassert_equal(sdhc_data.bytes_xfered, 512, "bytes_xfered mismatch");
}

/* Verify tuning block read returns the expected pattern. */
ZTEST(sdhc_emmc, test_tuning_block_pattern)
{
	struct sdhc_command cmd = {0};
	struct sdhc_data data = {0};
	uint8_t tuning_block[128] = {0};

	cmd.opcode = MMC_SEND_TUNING_BLOCK;
	data.data = tuning_block;
	zassert_ok(sdhc_request(dev, &cmd, &data), "CMD21 failed");
	zassert_equal(data.bytes_xfered, 128, "bytes_xfered mismatch for tuning");
	zassert_equal(tuning_block[0], 0xA5, "Tuning block mismatch");
	zassert_equal(tuning_block[127], 0xA5, "Tuning block mismatch");
}

/* Verify sleep state blocks data commands and awake restores operation. */
ZTEST(sdhc_emmc, test_sleep_awake)
{
	struct sdhc_command cmd = {0};

	cmd.opcode = SDIO_SEND_OP_COND;
	cmd.arg = 0x8000;
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD5 SLEEP failed");

	cmd.opcode = SD_READ_SINGLE_BLOCK;
	cmd.arg = 0;
	struct sdhc_data data = {0};

	zassert_not_equal(sdhc_request(dev, &cmd, &data), 0,
			  "Data command succeeded in SLEEP state");

	cmd.opcode = SDIO_SEND_OP_COND;
	cmd.arg = 0x0000;
	zassert_ok(sdhc_request(dev, &cmd, NULL), "CMD5 AWAKE failed");
}

/* Verify direct HS400 transition without HS200 is rejected. */
ZTEST(sdhc_emmc, test_hs400_without_hs200_rejected)
{
	struct sdhc_command cmd = {0};

	sdhc_init_to_transfer(dev);

	cmd.opcode = SD_SWITCH;
	cmd.arg = (3 << 24) | (EXT_CSD_HS_TIMING << 16) | (0x03 << 8);

	int ret = sdhc_request(dev, &cmd, NULL);

	zassert_equal(ret, -EINVAL, "Direct transition to HS400 must return -EINVAL");
}
