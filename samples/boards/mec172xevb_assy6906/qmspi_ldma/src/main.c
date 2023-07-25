/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

#define W25Q128_JEDEC_ID				0x001840efU
#define W25Q128JV_JEDEC_ID				0x001870efU
#define SST26VF016B_JEDEC_ID				0x004126bfU
#define SPI_FLASH_JEDEC_ID_MSK				0x00ffffffu

#define SPI_FLASH_READ_JEDEC_ID_OPCODE			0x9fu

#define SPI_FLASH_READ_STATUS1_OPCODE			0x05u
#define SPI_FLASH_READ_STATUS2_OPCODE			0x35u
#define SPI_FLASH_READ_STATUS3_OPCODE			0x15u

#define SPI_FLASH_WRITE_STATUS1_OPCODE			0x01u
#define SPI_FLASH_WRITE_STATUS2_OPCODE			0x31u
#define SPI_FLASH_WRITE_STATUS3_OPCODE			0x11u

#define SPI_FLASH_WRITE_ENABLE_OPCODE			0x06u
#define SPI_FLASH_WRITE_DISABLE_OPCODE			0x04u
#define SPI_FLASH_WRITE_ENABLE_VOLATILE_STS_OPCODE	0x50u

#define SPI_FLASH_READ_SFDP_CMD				0x5au

#define SPI_FLASH_READ_SLOW_OPCODE			0x03u
#define SPI_FLASH_READ_FAST_OPCODE			0x0bu
#define SPI_FLASH_READ_DUAL_OPCODE			0x3bu
#define SPI_FLASH_READ_QUAD_OPCODE			0x6bu

#define SPI_FLASH_ERASE_SECTOR_OPCODE			0x20u
#define SPI_FLASH_ERASE_BLOCK1_OPCODE			0x52u
#define SPI_FLASH_ERASE_BLOCK2_OPCODE			0xd8u
#define SPI_FLASH_ERASE_CHIP_OPCODE			0xc7u

#define SPI_FLASH_PAGE_PROG_OPCODE			0x02u

#define SPI_FLASH_PAGE_SIZE		256
#define SPI_FLASH_SECTOR_SIZE		4096
#define SPI_FLASH_BLOCK1_SIZE		(32 * 1024)
#define SPI_FLASH_BLOCK2_SIZE		(64 * 1024)

#define SPI_FLASH_STATUS1_BUSY_POS	0
#define SPI_FLASH_STATUS1_WEL_POS	1

#define SPI_FLASH_STATUS2_SRL_POS	0
#define SPI_FLASH_STATUS2_QE_POS	1
#define SPI_FLASH_STATUS2_SUS_STS_POS	7

#define SPI0_NODE			DT_NODELABEL(spi0)

struct spi_address {
	uint32_t addr;
	uint8_t byte_len;
};

enum spi_flash_read_cmd {
	CMD_SPI_FLASH_READ_SLOW = 0,
	CMD_SPI_FLASH_READ_FAST,
	CMD_SPI_FLASH_READ_DUAL,
	CMD_SPI_FLASH_READ_QUAD,
	CMD_SPI_FLASH_READ_MAX
};

static const struct device *spi_dev = DEVICE_DT_GET(SPI0_NODE);

static struct spi_config spi_cfg;
#ifdef CONFIG_SPI_EXTENDED_MODES
static struct spi_config spi_cfg_dual;
static struct spi_config spi_cfg_quad;
#endif

static uint32_t jedec_id;

#define SPI_TEST_BUF1_SIZE 256

static uint8_t buf1[SPI_TEST_BUF1_SIZE];

#define SPI_TEST_ADDR1 0xc10000u

#define SPI_FILL_VAL 0x69u
#define SPI_TEST_BUFFER_SIZE (36 * 1024)

struct test_buf {
	union {
		uint32_t w[(SPI_TEST_BUFFER_SIZE) / 4];
		uint16_t h[(SPI_TEST_BUFFER_SIZE) / 2];
		uint8_t b[(SPI_TEST_BUFFER_SIZE)];
	};
};

static struct test_buf tb1;

static bool is_data_buf_filled_with(uint8_t *data, size_t datasz, uint8_t val)
{
	if (!data || !datasz) {
		return false;
	}

	for (size_t i = 0; i < datasz; i++) {
		if (data[i] != val) {
			return false;
		}
	}

	return true;
}

static int spi_flash_format_addr(uint32_t spi_addr, size_t addrsz, uint8_t *buf, size_t bufsz)
{
	if (!buf || (addrsz > 4) || (bufsz < addrsz)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < addrsz; i++) {
		buf[i] = spi_addr >> ((addrsz - i - 1u) * 8);
	}

	return 0;
}

static int spi_flash_read_status(const struct device *dev, struct spi_config *spi_cfg,
				 uint8_t read_status_opcode, uint8_t *status)
{
	struct spi_buf spi_buffers[2];
	uint32_t cmdbuf;
	int err;

	if (!dev || !spi_cfg || !status) {
		return -EINVAL;
	}

	cmdbuf = read_status_opcode;
	spi_buffers[0].buf = &cmdbuf;
	spi_buffers[0].len = 1;
	spi_buffers[1].buf = status;
	spi_buffers[1].len = 1;

	const struct spi_buf_set tx_bufs = {
		.buffers = spi_buffers,
		.count = 2,
	};

	const struct spi_buf_set rx_bufs = {
		.buffers = spi_buffers,
		.count = 2,
	};

	err = spi_transceive(dev, spi_cfg, &tx_bufs, &rx_bufs);

	return err;
}

static int spi_poll_busy(const struct device *dev, struct spi_config *spi_cfg, uint32_t timeout_ms)
{
	int err = 0;
	uint32_t ms = 0;
	uint8_t spi_status = 0;

	if (!dev || !spi_cfg) {
		return -EINVAL;
	}

	spi_status = 0xffu;
	while (timeout_ms) {
		k_busy_wait(2);
		ms += 2u;
		spi_status = 0xffu;
		err = spi_flash_read_status(spi_dev, spi_cfg, SPI_FLASH_READ_STATUS1_OPCODE,
					    &spi_status);
		if (err) {
			return err;
		}

		if ((spi_status & BIT(SPI_FLASH_STATUS1_BUSY_POS)) == 0) {
			return 0;
		}
		if (ms > 1000) {
			timeout_ms--;
		}
	}

	return -ETIMEDOUT;
}

/* SPI flash full-duplex write of command opcode, optional command parameters, and optional data */
static int spi_flash_fd_wr_cpd(const struct device *dev, struct spi_config *spi_cfg, uint8_t cmd,
			       uint8_t *cmdparams, size_t cmdparamsz, uint8_t *data, size_t datasz)
{
	struct spi_buf tx_spi_buffers[3];
	int bufidx = 0, err = 0;
	uint8_t spicmd;

	if (!dev || !spi_cfg || (!cmdparams && cmdparamsz) || (!data && datasz)) {
		return -EINVAL;
	}

	spicmd = cmd;
	tx_spi_buffers[bufidx].buf = &spicmd;
	tx_spi_buffers[bufidx++].len = 1;
	if (cmdparams && cmdparamsz) {
		tx_spi_buffers[bufidx].buf = cmdparams;
		tx_spi_buffers[bufidx++].len = cmdparamsz;
	}
	if (data && datasz) {
		tx_spi_buffers[bufidx].buf = data;
		tx_spi_buffers[bufidx++].len = datasz;
	}

	const struct spi_buf_set tx_bufs = {
		.buffers = tx_spi_buffers,
		.count = bufidx,
	};

	err = spi_transceive(dev, spi_cfg, &tx_bufs, NULL);

	return err;
}

static int spi_flash_read_fd_sync(const struct device *dev, struct spi_config *spi_cfg,
				  struct spi_address *spi_addr, uint8_t *data, size_t datasz,
				  uint8_t opcode)
{
	uint8_t txdata[8] = {0};
	struct spi_buf spi_bufs[3] = {0};
	int cnt = 0, err = 0;
	size_t param_len = 0;

	if (!dev || !spi_cfg || !data || !datasz) {
		return -EINVAL;
	}

	txdata[0] = opcode;
	if (spi_addr) {
		if (spi_addr->byte_len > 4) {
			return -EINVAL;
		}
		err = spi_flash_format_addr(spi_addr->addr, spi_addr->byte_len,
					    &txdata[1], sizeof(txdata)-1);
		if (err) {
			return err;
		}
		param_len += spi_addr->byte_len;
	}

	spi_bufs[cnt].buf = txdata;
	spi_bufs[cnt++].len = param_len + 1;

	if (opcode == SPI_FLASH_READ_FAST_OPCODE) {
		spi_bufs[cnt].buf = NULL; /* 8 clocks with output tri-stated */
		spi_bufs[cnt++].len = 1;
	}

	spi_bufs[cnt].buf = data;
	spi_bufs[cnt++].len = datasz;

	const struct spi_buf_set txset = {
		.buffers = &spi_bufs[0],
		.count = cnt,
	};
	const struct spi_buf_set rxset = {
		.buffers = &spi_bufs[0],
		.count = cnt,
	};

	err = spi_transceive(dev, spi_cfg, &txset, &rxset);
	if (err) {
		return err;
	}

	return 0;
}

/* Dual or Quad read */
static int spi_flash_read_hd_sync(const struct device *dev, struct spi_config *spi_cfg_cmd,
				  struct spi_config *spi_cfg_data, struct spi_address *spi_addr,
				  uint8_t *data, size_t datasz, uint8_t opcode)
{
	uint8_t txdata[8] = {0};
	struct spi_buf spi_bufs[3] = {0};
	int err = 0;

	if (!dev || !spi_cfg_cmd || !spi_cfg_data || !data || !datasz
	    || !spi_addr || (spi_addr->byte_len > 4)) {
		return -EINVAL;
	}

	txdata[0] = opcode;
	err = spi_flash_format_addr(spi_addr->addr, spi_addr->byte_len,
				    &txdata[1], sizeof(txdata)-1);
	if (err) {
		return err;
	}

	spi_bufs[0].buf = txdata;
	spi_bufs[0].len = spi_addr->byte_len + 1;
	spi_bufs[1].buf = NULL;
	spi_bufs[1].len = 1u;
	spi_bufs[2].buf = data;
	spi_bufs[2].len = datasz;

	const struct spi_buf_set txset = {
		.buffers = &spi_bufs[0],
		.count = 2,
	};
	const struct spi_buf_set rxset = {
		.buffers = &spi_bufs[2],
		.count = 1,
	};

	spi_cfg_cmd->operation |= SPI_HOLD_ON_CS;
	err = spi_transceive(dev, spi_cfg_cmd, &txset, NULL);
	spi_cfg_cmd->operation &= ~SPI_HOLD_ON_CS;
	if (err) {
		return err;
	}

	err = spi_transceive(dev, spi_cfg_data, NULL, &rxset);

	return err;
}

static int spi_flash_erase_region(const struct device *dev, struct spi_config *spi_cfg,
				  uint8_t erase_opcode, struct spi_address *spi_addr,
				  int timeout_ms)
{
	int err = 0;
	uint8_t cmdparams[4] = {0};

	if (!dev || !spi_cfg || !spi_addr) {
		return -EINVAL;
	}

	err = spi_flash_format_addr(spi_addr->addr, spi_addr->byte_len,
				    cmdparams, sizeof(cmdparams));
	if (err) {
		return err;
	}

	err = spi_flash_fd_wr_cpd(dev, spi_cfg, erase_opcode, cmdparams,
				  spi_addr->byte_len, NULL, 0);
	if (err) {
		return err;
	}

	err = spi_poll_busy(dev, spi_cfg, timeout_ms);

	return err;
}

#ifdef CONFIG_SPI_ASYNC
#define NUM_ASYNC_SPI_BUFS 8

static volatile int spi_async_done;
static volatile int spi_async_status;
static volatile void *spi_async_userdata;

static struct spi_buf_set txbs;
static struct spi_buf_set rxbs;
static struct spi_buf sb[NUM_ASYNC_SPI_BUFS];
static uint8_t spi_async_txbuf[8];

static void spi_cb(const struct device *dev, int status, void *userdata)
{
	spi_async_status = status;
	spi_async_userdata = userdata;
	spi_async_done = 1;
}

static int spi_flash_read_fd_async(const struct device *dev, struct spi_config *spi_cfg,
				   struct spi_address *spi_addr, uint8_t *data, size_t datasz,
				   uint8_t opcode, spi_callback_t cb, void *userdata)
{
	int cnt = 0, err = 0;
	size_t param_len = 0;

	if (!dev || !spi_cfg || !data || !datasz) {
		return -EINVAL;
	}

	spi_async_txbuf[0] = opcode;
	if (spi_addr) {
		if (spi_addr->byte_len > 4) {
			return -EINVAL;
		}
		err = spi_flash_format_addr(spi_addr->addr, spi_addr->byte_len,
					    &spi_async_txbuf[1], sizeof(spi_async_txbuf)-1);
		if (err) {
			return err;
		}
		param_len += spi_addr->byte_len;
	}

	sb[cnt].buf = spi_async_txbuf;
	sb[cnt++].len = param_len + 1;

	if (opcode == SPI_FLASH_READ_FAST_OPCODE) {
		sb[cnt].buf = NULL; /* 8 clocks with output tri-stated */
		sb[cnt++].len = 1;
	}

	sb[cnt].buf = data;
	sb[cnt++].len = datasz;

	txbs.buffers = &sb[0];
	txbs.count = cnt;

	rxbs.buffers = &sb[0];
	rxbs.count = cnt;

	/* these parameters cannot be stack local to his function */
	err = spi_transceive_cb(dev, spi_cfg, &txbs, &rxbs, cb, userdata);
	if (err) {
		return err;
	}

	return 0;
}
#endif

int main(void)
{
	int err;
	uint32_t wait_count;
	size_t spi_len;
	struct spi_address spi_addr;
	uint8_t spi_status1, spi_status2;
	bool quad_enabled = false;

	spi_cfg.frequency = MHZ(24);
	spi_cfg.operation = SPI_WORD_SET(8) | SPI_LINES_SINGLE;
#ifdef CONFIG_SPI_EXTENDED_MODES
	spi_cfg_dual.frequency = spi_cfg.frequency;
	spi_cfg_dual.operation = SPI_WORD_SET(8) | SPI_LINES_DUAL;
	spi_cfg_quad.frequency = spi_cfg.frequency;
	spi_cfg_quad.operation = SPI_WORD_SET(8) | SPI_LINES_QUAD;
#endif
	if (!device_is_ready(spi_dev)) {
		printf("SPI 0 device not ready!\n");
		return -1;
	}

	memset(sb, 0, sizeof(sb));
	memset(buf1, 0, sizeof(buf1));
	memset((void *)&tb1, 0x55, sizeof(tb1));

	jedec_id = 0;
	err = spi_flash_read_fd_sync(spi_dev, &spi_cfg, NULL, (uint8_t *)&jedec_id, 3u,
				     SPI_FLASH_READ_JEDEC_ID_OPCODE);
	if (err) {
		printf("Read JEDEC_ID error %d\n", err);
		return err;
	}

	printf("JEDEC ID = 0x%08x\n", jedec_id);
	if (jedec_id == W25Q128_JEDEC_ID) {
		printf("W25Q128 16Mbyte SPI flash\n");
	} else if (jedec_id == W25Q128JV_JEDEC_ID) {
		printf("W25Q128JV 16Mbyte SPI flash\n");
	} else if (jedec_id == SST26VF016B_JEDEC_ID) {
		printf("SST26VF016B 16MByte SPI flash\n");
	} else {
		printf("Unknown SPI flash device\n");
		return -EIO;
	}

	spi_status1 = 0xffu;
	err = spi_flash_read_status(spi_dev, &spi_cfg, SPI_FLASH_READ_STATUS1_OPCODE,
				    &spi_status1);
	if (err) {
		printf("Read SPI flash Status1 error: %d\n", err);
		return err;
	}

	printf("SPI Flash Status1 = 0x%02x\n", spi_status1);

	spi_status2 = 0xffu;
	err = spi_flash_read_status(spi_dev, &spi_cfg, SPI_FLASH_READ_STATUS2_OPCODE,
				    &spi_status2);
	if (err) {
		printf("Read SPI flash STATUS2 error: %d\n", err);
		return err;
	}

	printf("SPI Flash Status2 = 0x%02x\n", spi_status2);
	if (spi_status2 & BIT(SPI_FLASH_STATUS2_QE_POS)) {
		quad_enabled = true;
		printf("Quad-Enable bit is set. WP# and HOLD# are IO[2] and IO[3]\n");
	}
	if (spi_status2 & BIT(SPI_FLASH_STATUS2_SRL_POS)) {
		printf("SPI Flash Status registers are locked!\n");
	}
	if (spi_status2 & BIT(SPI_FLASH_STATUS2_SUS_STS_POS)) {
		printf("SPI Flash is in Suspend state\n");
	}

	spi_addr.addr = SPI_TEST_ADDR1;
	spi_addr.byte_len = 3u;

	int num_sectors = 1u;

	if (SPI_TEST_BUFFER_SIZE > SPI_FLASH_SECTOR_SIZE) {
		num_sectors = SPI_TEST_BUFFER_SIZE / SPI_FLASH_SECTOR_SIZE;
	}

	for (int i = 0; i < num_sectors; i++) {
		printf("Transmit SPI flash Write-Enable\n");
		err = spi_flash_fd_wr_cpd(spi_dev, &spi_cfg, SPI_FLASH_WRITE_ENABLE_OPCODE,
					  NULL, 0, NULL, 0);
		if (err) {
			printf("ERROR: transmit SPI flash Write-Enable: error %d\n", err);
			return err;
		}

		printf("Transmit Erase-Sector at 0x%08x\n", spi_addr.addr);
		err = spi_flash_erase_region(spi_dev, &spi_cfg, SPI_FLASH_ERASE_SECTOR_OPCODE,
					     &spi_addr, 1000u);
		if (err) {
			printf("SPI flash erase sector error %d\n", err);
			return err;
		}

		spi_addr.addr += SPI_FLASH_SECTOR_SIZE;
	}

	printf("\nRead and check erased sectors using Read 1-1-1-0\n");

	spi_addr.addr = SPI_TEST_ADDR1;
	spi_addr.byte_len = 3u;

	for (int i = 0; i < num_sectors; i++) {
		memset(&tb1.b[0], 0x55, SPI_FLASH_SECTOR_SIZE);
		err = spi_flash_read_fd_sync(spi_dev, &spi_cfg, &spi_addr, &tb1.b[0],
					     SPI_FLASH_SECTOR_SIZE, SPI_FLASH_READ_SLOW_OPCODE);
		if (err) {
			printf("Read slow error %d\n", err);
			return err;
		}

		if (is_data_buf_filled_with(&tb1.b[0], SPI_FLASH_SECTOR_SIZE, 0xffu)) {
			printf("Sector at 0x%08x is erased\n", spi_addr.addr);
		} else {
			printf("FAIL: sector at 0x%08x is not erased\n", spi_addr.addr);
			return -1;
		}

		spi_addr.addr += SPI_FLASH_SECTOR_SIZE;
	}


	printf("Fill buffers for %d sectors and program flash region\n", num_sectors);

	for (int i = 0; i < SPI_TEST_BUFFER_SIZE; i++) {
		tb1.b[i] = SPI_FILL_VAL;
	}

	printf("\nProgram erased sectors\n");

	int num_pages = SPI_TEST_BUFFER_SIZE / SPI_FLASH_PAGE_SIZE;
	int offset = 0;

	spi_addr.addr = SPI_TEST_ADDR1;
	spi_addr.byte_len = 3u;

	for (int i = 0; i < num_pages; i++) {
		err = spi_flash_format_addr(spi_addr.addr, spi_addr.byte_len, buf1, sizeof(buf1));
		if (err) {
			printf("ERROR: format SPI address: %d\n", err);
			return err;
		}

		printf("Page at 0x%08x: Transmit SPI flash Write-Enable, "
			"Page-Program, and poll status\n", spi_addr.addr);
		err = spi_flash_fd_wr_cpd(spi_dev, &spi_cfg, SPI_FLASH_WRITE_ENABLE_OPCODE,
					  NULL, 0, NULL, 0);
		if (err) {
			printf("ERROR: transmit SPI flash Write-Enable: %d\n", err);
			return err;
		}

		err = spi_flash_fd_wr_cpd(spi_dev, &spi_cfg, SPI_FLASH_PAGE_PROG_OPCODE,
					  buf1, spi_addr.byte_len, &tb1.b[offset],
					  SPI_FLASH_PAGE_SIZE);
		if (err) {
			printf("ERROR: transmit SPI Page-Program: %d\n", err);
			return err;
		}

		err = spi_poll_busy(spi_dev, &spi_cfg, 100);
		if (err) {
			printf("ERROR: Poll SPI busy status: %d\n", err);
			return err;
		}

		spi_addr.addr += SPI_FLASH_PAGE_SIZE;
		offset += SPI_FLASH_PAGE_SIZE;
	}

	/* Read and check if programmed */
	spi_addr.addr = SPI_TEST_ADDR1;
	spi_addr.byte_len = 3u;
	spi_len = num_pages * SPI_FLASH_PAGE_SIZE;

	memset(&tb1.b[0], 0, sizeof(tb1));
	printf("Read %u bytes from 0x%08x using SPI Read 1-1-1-0\n", spi_len, spi_addr.addr);
	err = spi_flash_read_fd_sync(spi_dev, &spi_cfg, &spi_addr, &tb1.b[0], spi_len,
				     SPI_FLASH_READ_SLOW_OPCODE);
	if (err) {
		printf("Read slow error %d\n", err);
		return err;
	}

	if (is_data_buf_filled_with(&tb1.b[0], spi_len, SPI_FILL_VAL)) {
		printf("Data matches\n");
	} else {
		printf("FAIL: data mismatch\n");
		return -1;
	}

	memset(&tb1.b[0], 0, sizeof(tb1));
	printf("Read %u bytes from 0x%08x using SPI Read 1-1-1-8\n", spi_len, spi_addr.addr);
	err = spi_flash_read_fd_sync(spi_dev, &spi_cfg, &spi_addr, &tb1.b[0], spi_len,
				     SPI_FLASH_READ_FAST_OPCODE);
	if (err) {
		printf("Read fast error %d\n", err);
		return err;
	}

	if (is_data_buf_filled_with(&tb1.b[0], spi_len, SPI_FILL_VAL)) {
		printf("Data matches\n");
	} else {
		printf("FAIL: data mismatch\n");
		return -1;
	}

	memset(&tb1.b[0], 0, sizeof(tb1));
	printf("Read %u bytes from 0x%08x using SPI Read 1-1-2-8\n", spi_len, spi_addr.addr);
	err = spi_flash_read_hd_sync(spi_dev, &spi_cfg, &spi_cfg_dual, &spi_addr, &tb1.b[0],
				     spi_len, SPI_FLASH_READ_DUAL_OPCODE);
	if (err) {
		printf("Read dual error %d\n", err);
		return err;
	}

	if (is_data_buf_filled_with(&tb1.b[0], spi_len, SPI_FILL_VAL)) {
		printf("Data matches\n");
	} else {
		printf("FAIL: data mismatch\n");
		return -1;
	}

	memset(&tb1.b[0], 0, sizeof(tb1));
	printf("Read %u bytes from 0x%08x using SPI Read 1-1-4-8\n", spi_len, spi_addr.addr);
	err = spi_flash_read_hd_sync(spi_dev, &spi_cfg, &spi_cfg_quad, &spi_addr, &tb1.b[0],
				     spi_len, SPI_FLASH_READ_QUAD_OPCODE);
	if (err) {
		printf("Read quad error %d\n", err);
		return err;
	}

	if (is_data_buf_filled_with(&tb1.b[0], spi_len, SPI_FILL_VAL)) {
		printf("Data matches\n");
	} else {
		printf("FAIL: data mismatch\n");
		return -1;
	}

#ifdef CONFIG_SPI_ASYNC
	memset(&tb1.b[0], 0, sizeof(tb1));

	spi_async_status = 0;
	spi_async_userdata = 0;
	spi_async_done = 0;
	wait_count = 0;

	err = spi_flash_read_fd_async(spi_dev, &spi_cfg, &spi_addr, &tb1.b[0], spi_len,
				      SPI_FLASH_READ_FAST_OPCODE, spi_cb,
				      (void *)spi_async_userdata);
	if (err) {
		printf("ERROR: SPI async full-duplex error: %d\n", err);
		return err;
	}

	while (spi_async_done == 0) {
		wait_count++;
	}

	printf("SPI Async read done: wait count = %u\n", wait_count);
	printf("SPI Async done = %d\n", spi_async_done);
	printf("SPI Async status = %d\n", spi_async_status);
	printf("SPI Async userdata = %p\n", spi_async_userdata);

	if (is_data_buf_filled_with(&tb1.b[0], spi_len, SPI_FILL_VAL)) {
		printf("Data matches\n");
	} else {
		printf("FAIL: data mismatch\n");
		return -1;
	}
#endif

	printf("\nProgram End\n");
	return 0;
}
