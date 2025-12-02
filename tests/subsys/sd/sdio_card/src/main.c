/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/sd/sdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/ztest.h>

#define BLOCK_SIZE    (256)
#define BLOCK_CNT_MAX (32)
#define BUF_SIZE      (BLOCK_CNT_MAX * BLOCK_SIZE)

/*
 * SDIO card specific buffer addresses.
 * These addresses are vendor-specific and may need adjustment
 * for other SDIO card types.
 */
#define RSI_PING_BUFFER_ADDR 0x18000

#define BYTE_TEST_OFFSET        4U
#define DATA_PATTERN_0X5555AAAA 0x5555AAAAU
#define DATA_PATTERN_0XFFFF0000 0xFFFF0000U
#define WALKING_PATTERN_BITS    8U

static const struct device *sdhc_dev = DEVICE_DT_GET(DT_ALIAS(sdhc1));
static struct sd_card card;
static struct sdio_func sdio_func1;

static uint8_t buf[BUF_SIZE] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
static uint8_t check_buf[BUF_SIZE] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);

/*
 * @brief Prepare a data pattern for SDIO write testing
 *
 * Generates various data patterns for testing SDIO write/read operations.
 * Patterns include alternating bits, walking bits, and incremental/decremental sequences.
 *
 * @param pattern_index Index of the pattern to generate (0-4)
 *                     0: 0x5555AAAA pattern
 *                     1: 0xFFFF0000 pattern
 *                     2: Walking bit pattern
 *                     3: Incremental from 1
 *                     4: Decremental from 0xFF
 * @param buff Buffer to fill with pattern data (must not be NULL)
 * @param len Length of buffer in bytes (must be > 0)
 */
void prepare_data_pattern(uint32_t pattern_index, uint8_t *buff, uint32_t len)
{
	if (buff == NULL || len == 0) {
		return;
	}

	uint32_t *pui32TxPtr = (uint32_t *)buff;
	uint8_t *pui8TxPtr = (uint8_t *)buff;

	switch (pattern_index) {
	case 0:
		/* 0x5555AAAA */
		for (uint32_t i = 0; i < len / 4; i++) {
			pui32TxPtr[i] = DATA_PATTERN_0X5555AAAA;
		}
		break;
	case 1:
		/* 0xFFFF0000 */
		for (uint32_t i = 0; i < len / 4; i++) {
			pui32TxPtr[i] = DATA_PATTERN_0XFFFF0000;
		}
		break;

	case 2:
		/* walking */
		for (uint32_t i = 0; i < len; i++) {
			pui8TxPtr[i] = 0x01 << (i % WALKING_PATTERN_BITS);
		}
		break;
	case 3:
		/* incremental from 1 */
		for (uint32_t i = 0; i < len; i++) {
			pui8TxPtr[i] = ((i + 1) & 0xFF);
		}
		break;
	case 4:
		/* decremental from 0xff */
		for (uint32_t i = 0; i < len; i++) {
			pui8TxPtr[i] = (0xff - i) & 0xFF;
		}
		break;
	default:
		/* incremental from 1 */
		for (uint32_t i = 0; i < len; i++) {
			pui8TxPtr[i] = ((i) & 0xFF);
		}
		break;
	}
}

/*
 * Verify that SD stack can initialize an SDIO card.
 * This test must run first, to ensure the card is initialized.
 */
ZTEST(sd_stack, test_0_init)
{
	int ret;

	/* SD common init */
	zassert_true(device_is_ready(sdhc_dev), "SDHC device is not ready");

	ret = sd_is_card_present(sdhc_dev);
	zassert_equal(ret, 1, "SD card not present in slot");

	ret = sd_init(sdhc_dev, &card);
	zassert_equal(ret, 0, "Card initialization failed");

	/* SDIO card specific init */
	zassert_equal(card.type, CARD_SDIO, "Card is not SDIO type");
	ret = sdio_init_func(&card, &sdio_func1, SDIO_FUNC_NUM_1);
	zassert_equal(ret, 0, "SDIO Card function initialization failed");

	ret = sdio_enable_func(&sdio_func1);
	zassert_equal(ret, 0, "SDIO Card function enable failed");

	ret = sdio_enable_interrupt(&sdio_func1);
	zassert_equal(ret, 0, "SDIO Card function interrupt enable failed");

	ret = sdio_set_block_size(&sdio_func1, BLOCK_SIZE);
	zassert_equal(ret, 0, "SDIO card set block size failed");
}

/* Simply dump the card configuration. */
ZTEST(sd_stack, test_card_config)
{
	switch (card.card_voltage) {
	case SD_VOL_1_2_V:
		TC_PRINT("Card voltage: 1.2V\n");
		break;
	case SD_VOL_1_8_V:
		TC_PRINT("Card voltage: 1.8V\n");
		break;
	case SD_VOL_3_0_V:
		TC_PRINT("Card voltage: 3.0V\n");
		break;
	case SD_VOL_3_3_V:
		TC_PRINT("Card voltage: 3.3V\n");
		break;
	default:
		zassert_unreachable("Card voltage is not known value");
	}
	zassert_equal(card.status, CARD_INITIALIZED, "Card status is not OK");
	switch (card.card_speed) {
	case SD_TIMING_SDR12:
		TC_PRINT("Card timing: SDR12\n");
		break;
	case SD_TIMING_SDR25:
		TC_PRINT("Card timing: SDR25\n");
		break;
	case SD_TIMING_SDR50:
		TC_PRINT("Card timing: SDR50\n");
		break;
	case SD_TIMING_SDR104:
		TC_PRINT("Card timing: SDR104\n");
		break;
	case SD_TIMING_DDR50:
		TC_PRINT("Card timing: DDR50\n");
		break;
	default:
		zassert_unreachable("Card timing is not known value");
	}
	switch (card.type) {
	case CARD_SDIO:
		TC_PRINT("Card type: SDIO\n");
		break;
	case CARD_SDMMC:
		TC_PRINT("Card type: SDMMC\n");
		break;
	case CARD_COMBO:
		TC_PRINT("Card type: combo card\n");
		break;
	default:
		zassert_unreachable("Card type is not known value");
	}
	if (card.sd_version >= SD_SPEC_VER3_0) {
		TC_PRINT("Card spec: 3.0\n");
	} else if (card.sd_version >= SD_SPEC_VER2_0) {
		TC_PRINT("Card spec: 2.0\n");
	} else if (card.sd_version >= SD_SPEC_VER1_1) {
		TC_PRINT("Card spec: 1.1\n");
	} else if (card.sd_version >= SD_SPEC_VER1_0) {
		TC_PRINT("Card spec: 1.0\n");
	} else {
		zassert_unreachable("Card spec is unknown value");
	}
}

/* Verify multiple bytes transfer on SDIO devices */
ZTEST(sd_stack, test_write_read_bytes)
{
	int ret;
	int32_t len, loopcnt = 0;

	for (len = 4; len <= BLOCK_SIZE; len += BYTE_TEST_OFFSET) {
		/* Zero the write buffer */
		memset(buf, 0, len);
		memset(check_buf, 0, len);
		prepare_data_pattern(loopcnt % 5, buf, len);
		TC_PRINT("SDIO bytes test len:%d\n", len);

		/* Write bytes to card FIFO area. */
		ret = sdio_write_fifo(&sdio_func1, RSI_PING_BUFFER_ADDR, buf, len);
		zassert_equal(ret, 0, "SDIO card write failed");

		/* Read after write to verify that data was written */
		ret = sdio_read_fifo(&sdio_func1, RSI_PING_BUFFER_ADDR, check_buf, len);
		zassert_equal(ret, 0, "Read from SDIO card failed");

		zassert_mem_equal(buf, check_buf, len,
			"SDIO Card bytes test:read data does not match written data");
		loopcnt++;
	}
}

/* Verify multiple blocks transfer on SDIO devices */
ZTEST(sd_stack, test_write_read_multiple_blocks)
{
	int ret;
	int block_num, len = 0;

	for (block_num = 1; block_num <= BLOCK_CNT_MAX; block_num++) {
		len = block_num * BLOCK_SIZE;

		/* Zero the write buffer */
		memset(buf, 0, len);
		memset(check_buf, 0, len);
		prepare_data_pattern(block_num % 5, buf, len);
		TC_PRINT("SDIO Card multiple block count:%d len:%d\n", block_num, len);

		/* Write block_num blocks to card FIFO area. */
		ret = sdio_write_blocks_fifo(&sdio_func1, RSI_PING_BUFFER_ADDR, buf, block_num);
		zassert_equal(ret, 0, "SDIO card multiple blocks write failed");

		/* Read after write to verify that data was written */
		ret = sdio_read_blocks_fifo(&sdio_func1, RSI_PING_BUFFER_ADDR, check_buf,
					    block_num);
		zassert_equal(ret, 0, "Multiple blocks read from SDIO card failed");

		zassert_mem_equal(buf, check_buf, len,
			 "SDIO Card Multiple blocks test:read data does not match written data");
	}
}

ZTEST_SUITE(sd_stack, NULL, NULL, NULL, NULL, NULL);
