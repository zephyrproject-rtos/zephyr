/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/mmc.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* limit the max test mmc sector count and transfer data */
#define BLK_CNT     128
/* subsystem should set all cards to 512 byte blocks */
#define SECTOR_SIZE 512
#define BUF_SIZE    (SECTOR_SIZE * BLK_CNT)

/* freely define the number of loops*/
#define LOOP_CNT (3)

#define SPEED_START_INDEX (0)
#define SPEED_END_INDEX   (2)

#define WIDTH_START_INDEX (0)
#define WIDTH_END_INDEX   (2)

#define STACK_SIZE (1024 * 5)

#define DATA_PATTERN_0X5555AAAA 0x5555AAAAU
#define DATA_PATTERN_0XFFFF0000 0xFFFF0000U
#define WALKING_PATTERN_BITS 8U

static const struct device *const sdhc0_dev = DEVICE_DT_GET(DT_ALIAS(sdhc0));
static const struct device *const sdhc1_dev = DEVICE_DT_GET(DT_ALIAS(sdhc1));

static struct sd_card mmc0_card;
static struct sd_card mmc1_card;

static uint8_t buf[BUF_SIZE] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
static uint8_t check_buf[BUF_SIZE] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);

static uint8_t sdhc1_buf[BUF_SIZE] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
static uint8_t sdhc1_check_buf[BUF_SIZE] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);

K_THREAD_STACK_DEFINE(my_stack_area, STACK_SIZE);
K_THREAD_STACK_DEFINE(my_stack_area_0, STACK_SIZE);

K_SEM_DEFINE(mmc0_sem, 0, 1);
K_SEM_DEFINE(mmc1_sem, 0, 1);
K_SEM_DEFINE(multiple_mmc_end, 0, 2);

struct k_thread mmc0_thread;
struct k_thread mmc1_thread;

typedef struct {
	enum sdhc_bus_width width;
	const char *string;
} sdio_width_t;

typedef struct {
	const uint32_t speed;
	const char *string;
} sdio_speed_t;

sdio_speed_t sdio_test_speeds[] = {
	{96000000, "96MHz"}, {48000000, "48MHz"}, {24000000, "24MHz"}, {12000000, "12MHz"},
	{3000000, "3MHz"},   {750000, "750KHz"},  {375000, "375KHz"},
};

sdio_width_t sdio_test_widths[] = {
	{SDHC_BUS_WIDTH4BIT, "4bit"},
	{SDHC_BUS_WIDTH8BIT, "8bit"},
};

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

/* Test reads and writes in different mode settings on sdhc0 */
static void mmc0_wr_test(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int ret, err;
	int block_addr = 0;
	int blk_cnt = 0;
	int loopcnt = LOOP_CNT;
	int speed_index, bitwidth = 0;
	uint32_t sector_count;

	zassert_true(device_is_ready(sdhc0_dev), "SDHC0 device is not ready");

	mmc0_card.bus_width = sdio_test_widths[bitwidth].width;
	ret = sd_init(sdhc0_dev, &mmc0_card);
	zassert_equal(ret, 0, "mmc0 card initialization failed");

	ret = mmc_ioctl(&mmc0_card, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	zassert_equal(ret, 0, "IOCTL sector count read failed");
	TC_PRINT("MMC0 reports sector count of %d\n", sector_count);

	/* Zero the write buffer */
	memset(buf, 0, BUF_SIZE);
	memset(check_buf, 0, BUF_SIZE);
	ret = mmc_write_blocks(&mmc0_card, buf, block_addr, BLK_CNT / 2);
	zassert_equal(ret, 0, "Write to mmc0 card failed");
	/* Verify that a read from this area is empty */
	ret = mmc_read_blocks(&mmc0_card, buf, block_addr, BLK_CNT / 2);
	zassert_equal(ret, 0, "Read from mmc0 card failed");
	zassert_mem_equal(buf, check_buf, SECTOR_SIZE * BLK_CNT / 2,
			  "Read of erased area was not zero");

	while (loopcnt--) {
		for (bitwidth = WIDTH_START_INDEX; bitwidth < WIDTH_END_INDEX; bitwidth++) {
			for (speed_index = SPEED_START_INDEX; speed_index < SPEED_END_INDEX;
			     speed_index++) {

				mmc0_card.bus_width = sdio_test_widths[bitwidth].width;
				mmc0_card.bus_io.clock = sdio_test_speeds[speed_index].speed;

				if ((sdio_test_speeds[speed_index].speed == 96000000 &&
				     mmc0_card.host_props.host_caps.hs200_support == false) ||
				    (sdio_test_speeds[speed_index].speed == 48000000 &&
				     mmc0_card.host_props.host_caps.hs200_support == true)) {
					continue;
				}

				ret = sd_init(sdhc0_dev, &mmc0_card);
				zassert_equal(ret, 0, "mmc0 init failed");

				TC_PRINT("MMC0 write read test width:%s speed:%s\n",
					 sdio_test_widths[bitwidth].string,
					 sdio_test_speeds[speed_index].string);

				for (blk_cnt = 32; blk_cnt <= BLK_CNT; blk_cnt += BLK_CNT / 4) {
					for (block_addr = 0; block_addr < sector_count;
					     block_addr += sector_count / 4) {
						TC_PRINT("MMC0 write read start block "
							 " 0x%x, block cnt = %d\n",
							 block_addr, blk_cnt);
						k_sem_give(&mmc0_sem);
						/* Prepare data pattern */
						prepare_data_pattern(loopcnt % 5, check_buf,
								     blk_cnt * SECTOR_SIZE);

						ret = mmc_write_blocks(&mmc0_card, check_buf,
								       block_addr, blk_cnt);
						zassert_equal(ret, 0,
							      "Write to mmc0 card failed\n");
						/* Clear the read buffer */
						memset(buf, 0, blk_cnt * SECTOR_SIZE);
						ret = mmc_read_blocks(&mmc0_card, buf, block_addr,
								      blk_cnt);
						zassert_equal(ret, 0,
							      "Read from mmc0 card failed\n");
						zassert_mem_equal(buf, check_buf,
								  SECTOR_SIZE * blk_cnt,
								  "Read data was not correct\n");
						err = k_sem_take(&mmc1_sem, K_FOREVER);
						zassert_equal(err, 0, "k_sem_take failed %d", err);
					}
				}
			}
		}
	}
	k_sem_give(&multiple_mmc_end);
}

/* Test reads and writes in different mode settings on sdhc1 */
static void mmc1_wr_test(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int ret, err;
	int block_addr = 0;
	int blk_cnt = 0;
	int loopcnt = LOOP_CNT;
	int speed_index, bitwidth = 0;
	uint32_t sector_count;

	zassert_true(device_is_ready(sdhc1_dev), "SDHC1 device is not ready");

	mmc1_card.bus_width = sdio_test_widths[bitwidth].width;
	ret = sd_init(sdhc1_dev, &mmc1_card);
	zassert_equal(ret, 0, "mmc1 card initialization failed");

	ret = mmc_ioctl(&mmc1_card, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	zassert_equal(ret, 0, "IOCTL sector count read failed");
	TC_PRINT("MMC1 reports sector count of %d\n", sector_count);

	/* Zero the write buffer */
	memset(sdhc1_buf, 0, BUF_SIZE);
	memset(sdhc1_check_buf, 0, BUF_SIZE);
	ret = mmc_write_blocks(&mmc1_card, sdhc1_buf, block_addr, BLK_CNT / 2);
	zassert_equal(ret, 0, "Write to mmc1 card failed");
	/* Verify that a read from this area is empty */
	ret = mmc_read_blocks(&mmc1_card, sdhc1_buf, block_addr, BLK_CNT / 2);
	zassert_equal(ret, 0, "Read from mmc1 card failed");
	zassert_mem_equal(sdhc1_buf, sdhc1_check_buf, SECTOR_SIZE * BLK_CNT / 2,
			  "Read of erased area was not zero");

	while (loopcnt--) {
		for (bitwidth = WIDTH_START_INDEX; bitwidth < WIDTH_END_INDEX; bitwidth++) {
			for (speed_index = SPEED_START_INDEX; speed_index < SPEED_END_INDEX;
			     speed_index++) {

				mmc1_card.bus_width = sdio_test_widths[bitwidth].width;
				mmc1_card.bus_io.clock = sdio_test_speeds[speed_index].speed;

				if ((sdio_test_speeds[speed_index].speed == 96000000 &&
				     mmc1_card.host_props.host_caps.hs200_support == false) ||
				    (sdio_test_speeds[speed_index].speed == 48000000 &&
				     mmc1_card.host_props.host_caps.hs200_support == true)) {
					continue;
				}

				ret = sd_init(sdhc1_dev, &mmc1_card);
				zassert_equal(ret, 0, "mmc1 init failed");

				TC_PRINT("MMC1 write read test width:%s speed:%s\n",
					 sdio_test_widths[bitwidth].string,
					 sdio_test_speeds[speed_index].string);

				for (blk_cnt = 32; blk_cnt <= BLK_CNT; blk_cnt += BLK_CNT / 4) {
					for (block_addr = 0; block_addr < sector_count;
					     block_addr += sector_count / 4) {
						TC_PRINT("MMC1 write read start block "
							 " 0x%x, block cnt = %d\n",
							 block_addr, blk_cnt);

						err = k_sem_take(&mmc0_sem, K_FOREVER);
						zassert_equal(err, 0, "k_sem_take failed %d", err);

						/* Prepare data block */
						prepare_data_pattern(loopcnt % 5, sdhc1_check_buf,
								     blk_cnt * SECTOR_SIZE);

						ret = mmc_write_blocks(&mmc1_card, sdhc1_check_buf,
								       block_addr, blk_cnt);
						zassert_equal(ret, 0,
							      "Write to mmc1 card failed\n");

						/* Clear the read buffer */
						memset(sdhc1_buf, 0, blk_cnt * SECTOR_SIZE);
						ret = mmc_read_blocks(&mmc1_card, sdhc1_buf,
								      block_addr, blk_cnt);
						zassert_equal(ret, 0,
							      "Read from mmc1 card failed\n");
						zassert_mem_equal(sdhc1_buf, sdhc1_check_buf,
								  SECTOR_SIZE * blk_cnt,
								  "Read data was not correct\n");
						k_sem_give(&mmc1_sem);
					}
				}
			}
		}
	}
	k_sem_give(&multiple_mmc_end);
}

void run_thread_system(void)
{
	int err;
	k_tid_t yield0_tid;
	k_tid_t yield1_tid;

	k_sleep(K_MSEC(10));

	yield0_tid = k_thread_create(&mmc0_thread, my_stack_area, STACK_SIZE, mmc0_wr_test, NULL,
				     NULL, NULL, 0 /*priority*/, 0, K_NO_WAIT);
	zassert_not_null(yield0_tid, "MMC 0 creation failed");

	yield1_tid = k_thread_create(&mmc1_thread, my_stack_area_0, STACK_SIZE, mmc1_wr_test, NULL,
				     NULL, NULL, 0 /*priority*/, 0, K_NO_WAIT);
	zassert_not_null(yield1_tid, "MMC1 1 creation failed");

	k_sleep(K_MSEC(8000));

	err = k_sem_take(&multiple_mmc_end, K_FOREVER);
	zassert_equal(err, 0, "MMC0 thread completion failed");
	err = k_sem_take(&multiple_mmc_end, K_FOREVER);
	zassert_equal(err, 0, "MMC1 thread completion failed");

	/* Clear the spawn threads */
	k_thread_abort(yield0_tid);
	k_thread_abort(yield1_tid);

	k_thread_join(yield0_tid, K_FOREVER);
	k_thread_join(yield1_tid, K_FOREVER);
}

ZTEST(sd_stack, test_mmc_multiple)
{
	TC_PRINT("MMC multiple write read test start\n");

	run_thread_system();

	TC_PRINT("MMC multiple write read test complete\n");
}

ZTEST_SUITE(sd_stack, NULL, NULL, NULL, NULL, NULL);
