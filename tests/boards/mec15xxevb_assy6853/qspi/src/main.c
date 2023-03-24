/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/spi.h>

#define TEST_FREQ_HZ 24000000U
#define W25Q128_JEDEC_ID 0x001840efU

#define TEST_BUF_SIZE 4096U
#define MAX_TX_BUF 2

#define SPI_STATUS2_QE 0x02U
#define SPI_READ_JEDEC_ID 0x9FU
#define SPI_READ_STATUS1 0x05U
#define SPI_READ_STATUS2 0x35U
#define SPI_WRITE_STATUS2 0x31U
#define SPI_WRITE_ENABLE_VS 0x50U
#define SPI_WRITE_ENABLE 0x06U
#define SPI_SECTOR_ERASE 0x20U
#define SPI_SINGLE_WRITE_DATA 0x02U
#define SPI_QUAD_WRITE_DATA 0x32U

/* bits[7:0] = spi opcode,
 * bits[15:8] = bytes number of dummy clocks */
#define SPI_FAST_READ_DATA 0x080BU
#define SPI_DUAL_FAST_READ_DATA 0x083BU
#define SPI_QUAD_FAST_READ_DATA 0x086BU
#define SPI_OCTAL_QUAD_READ_DATA 0xE3U

#define BUF_SIZE 11
uint8_t buffer_tx[] = "0123456789\0";
#define BUF_SIZE_2 7
uint8_t buffer_tx_2[] = "abcdef\0";

#define SPI_TEST_ADDRESS 0x000010U
#define SPI_TEST_ADDRESS_2 0x000020U

static uint8_t safbuf[TEST_BUF_SIZE] __aligned(4);
static uint8_t safbuf2[TEST_BUF_SIZE] __aligned(4);
static const struct device *const spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi0));
struct spi_buf_set tx_bufs, rx_bufs;
struct spi_buf txb[MAX_TX_BUF], rxb;
struct spi_config spi_cfg_single, spi_cfg_dual, spi_cfg_quad;


static void spi_single_init(void)
{
	/* configure spi as single mode */
	spi_cfg_single.frequency = TEST_FREQ_HZ;
	spi_cfg_single.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB
		| SPI_WORD_SET(8) | SPI_LINES_SINGLE;
	spi_cfg_single.slave = 0;
	spi_cfg_single.cs = NULL;

	zassert_true(device_is_ready(spi_dev), "SPI controller device is not ready");
}

/**
 * @brief Test spi device
 * @details
 * - Find spi device
 * - Read flash jedec id
 */
ZTEST_USER(spi, test_spi_device)
{
	uint32_t jedec_id;
	int ret;

	/* read jedec id */
	memset(safbuf, 0, TEST_BUF_SIZE);
	safbuf[0] = SPI_READ_JEDEC_ID;
	txb[0].buf = &safbuf;
	txb[0].len = 1U;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	jedec_id = 0U;
	rxb.buf = &jedec_id;
	rxb.len = 3U;

	rx_bufs.buffers = (const struct spi_buf *)&rxb;
	rx_bufs.count = 1U;

	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Read JEDEC ID spi_transceive failure: "
			"error %d", ret);
	zassert_true(jedec_id == W25Q128_JEDEC_ID, "JEDEC ID doesn't match");
}

/**
 * @brief Test spi sector erase
 * @details
 * - write enable
 * - erase data in flash device
 * - read register1 and wait for erase operation completed
 */
ZTEST_USER(spi_sector_erase, test_spi_sector_erase)
{
	int ret;

	/* write enable */
	memset(safbuf, 0, TEST_BUF_SIZE);
	safbuf[0] = SPI_WRITE_ENABLE;
	txb[0].buf = &safbuf;
	txb[0].len = 1U;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	rx_bufs.buffers = NULL;
	rx_bufs.count = 0U;

	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Send write enable spi_transceive failure: "
			"error %d", ret);

	/* erase data start from address 0x000010 */
	safbuf[0] = SPI_SECTOR_ERASE;
	safbuf[1] = SPI_TEST_ADDRESS & 0xFFFFFFU;

	txb[0].buf = &safbuf;
	txb[0].len = 4;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	rx_bufs.buffers = NULL;
	rx_bufs.count = 0U;

	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Send sector erase data spi_transceive failure: "
			"error %d", ret);

	/* read register1 to check whether erase operation completed */
	safbuf[0] = SPI_READ_STATUS1;

	txb[0].buf = &safbuf;
	txb[0].len = 1;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	memset(safbuf2, 1U, 1U);
	rxb.buf = &safbuf2;
	rxb.len = 1U;

	rx_bufs.buffers = (const struct spi_buf *)&rxb;
	rx_bufs.count = 1U;

	/* waiting for erase operation completed */
	while (safbuf2[0]) {
		ret = spi_transceive(spi_dev,
				(const struct spi_config *)&spi_cfg_single,
				(const struct spi_buf_set *)&tx_bufs,
				(const struct spi_buf_set *)&rx_bufs);
		zassert_true(ret == 0, "Send read register1 spi_transceive "
				"failure: error %d", ret);
	}
}

/**
 * @brief Write data into flash using spi api
 * @details
 * - flash write enable
 * - write data into flash using spi api
 */
static void test_spi_single_write(void)
{
	int ret;

	/* write enable */
	memset(safbuf, 0, TEST_BUF_SIZE);
	safbuf[0] = SPI_WRITE_ENABLE;
	txb[0].buf = &safbuf;
	txb[0].len = 1U;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	rx_bufs.buffers = NULL;
	rx_bufs.count = 0U;

	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Send write enable spi_transceive failure: "
			"error %d", ret);

	/* write data start from address 0x000010 */
	safbuf[0] = SPI_SINGLE_WRITE_DATA;
	safbuf[1] = SPI_TEST_ADDRESS & 0xFFFFFFU;
	memcpy(&safbuf[4], buffer_tx, BUF_SIZE);

	txb[0].buf = &safbuf;
	txb[0].len = 4 + BUF_SIZE;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	rx_bufs.buffers = NULL;
	rx_bufs.count = 0U;

	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Send write data spi_transceive failure: "
			"error %d", ret);
}

/**
 * @brief Read data from flash using spi single mode
 * @details
 * - read data using spi single mode
 * - check read buffer data whether correct
 */
ZTEST_USER(spi, test_spi_single_read)
{
	int ret;
	uint8_t cnt = 0;
	uint16_t spi_opcode;

	spi_opcode = SPI_FAST_READ_DATA;

	/* read data using spi single mode */
	/* set the spi operation code and address */
	memset(safbuf, 0, TEST_BUF_SIZE);
	safbuf[0] = spi_opcode & 0xFFU;
	safbuf[1] = SPI_TEST_ADDRESS & 0xFFFFFFU;

	txb[cnt].buf = &safbuf;
	txb[cnt].len = 4U;

	/* set the dummy clocks */
	if (spi_opcode & 0xFF00U) {
		cnt++;
		txb[cnt].buf = NULL;
		txb[cnt].len = (spi_opcode >> 8) & 0xFFU;
	}

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = cnt + 1;

	memset(safbuf2, 0, BUF_SIZE);
	rxb.buf = &safbuf2;
	rxb.len = BUF_SIZE;

	rx_bufs.buffers = (const struct spi_buf *)&rxb;
	rx_bufs.count = 1U;

	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Send fast read data spi_transceive failure: "
			"error %d", ret);

	/* check read buffer data whether correct */
	zassert_true(memcmp(buffer_tx, safbuf2, BUF_SIZE) == 0,
			"Buffer read data is different to write data");
}

static void spi_dual_init(void)
{
	/* configure spi dual mode */
	spi_cfg_dual.frequency = TEST_FREQ_HZ;
	spi_cfg_dual.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB
		| SPI_WORD_SET(8) | SPI_LINES_DUAL;
	spi_cfg_dual.slave = 0;
	spi_cfg_dual.cs = NULL;

	zassert_true(device_is_ready(spi_dev), "SPI controller device is not ready");
}

/**
 * @brief Read data from flash using spi dual mode
 * @details
 * - read data using spi dual mode
 * - check read buffer data whether correct
 */
ZTEST_USER(spi, test_spi_dual_read)
{
	int ret;
	uint8_t cnt = 0;
	uint16_t spi_opcode;

	spi_dual_init();

	spi_opcode = SPI_DUAL_FAST_READ_DATA;

	/* read data using spi dual mode */
	/* set the spi operation code and address */
	memset(safbuf, 0, TEST_BUF_SIZE);
	safbuf[0] = spi_opcode & 0xFFU;
	safbuf[1] = SPI_TEST_ADDRESS & 0xFFFFFFU;

	txb[cnt].buf = &safbuf;
	txb[cnt].len = 4U;

	/* set the dummy clocks */
	if (spi_opcode & 0xFF00U) {
		cnt++;
		txb[cnt].buf = NULL;
		txb[cnt].len = (spi_opcode >> 8) & 0xFFU;
	}

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = cnt + 1;

	/* send opcode and address using single mode */
	spi_cfg_single.operation |= SPI_HOLD_ON_CS;
	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs, NULL);
	zassert_true(ret == 0, "Send fast read data spi_transceive failure: "
			"error %d", ret);

	memset(safbuf2, 0, BUF_SIZE);
	rxb.buf = &safbuf2;
	rxb.len = BUF_SIZE;

	rx_bufs.buffers = (const struct spi_buf *)&rxb;
	rx_bufs.count = 1U;

	/* get read data using dual mode */
	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_dual,
			NULL, (const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Receive fast read data spi_transceive failure: "
			"error %d", ret);

	/* check read buffer data whether correct */
	zassert_true(memcmp(buffer_tx, safbuf2, BUF_SIZE) == 0,
			"Buffer read data is different to write data");

	/* release spi device */
	spi_cfg_single.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB
		| SPI_WORD_SET(8) | SPI_LINES_SINGLE;
	ret = spi_release(spi_dev, (const struct spi_config *)&spi_cfg_single);
	zassert_true(ret == 0, "Spi release failure: error %d", ret);
}

/**
 * @brief Write data into flash using spi quad mode
 * @details
 * - check and make sure spi quad mode is enabled
 * - write data using spi quad mode
 */
static void test_spi_quad_write(void)
{
	int ret;
	uint8_t spi_status2;

	/* read register2 to judge whether quad mode is enabled */
	safbuf[0] = SPI_READ_STATUS2;
	txb[0].buf = &safbuf;
	txb[0].len = 1U;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	memset(safbuf2, 0, 1U);
	rxb.buf = &safbuf2;
	rxb.len = 1U;

	rx_bufs.buffers = (const struct spi_buf *)&rxb;
	rx_bufs.count = 1U;

	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Read register2 status spi_transceive failure: "
			"error %d", ret);

	spi_status2 = safbuf2[0];
	/* set register2 QE=1 to enable quad mode */
	if ((spi_status2 & SPI_STATUS2_QE) == 0U) {
		safbuf[0] = SPI_WRITE_ENABLE_VS;

		txb[0].buf = &safbuf;
		txb[0].len = 1U;

		tx_bufs.buffers = (const struct spi_buf *)&txb[0];
		tx_bufs.count = 1U;

		rx_bufs.buffers = NULL;
		rx_bufs.count = 0U;

		ret = spi_transceive(spi_dev,
				(const struct spi_config *)&spi_cfg_single,
				(const struct spi_buf_set *)&tx_bufs,
				(const struct spi_buf_set *)&rx_bufs);
		zassert_true(ret == 0,
				"Send write enable volatile spi_transceive failure: "
				"error %d", ret);

		safbuf[0] = SPI_WRITE_STATUS2;
		safbuf[1] = spi_status2 | SPI_STATUS2_QE;

		txb[0].buf = &safbuf;
		txb[0].len = 2U;

		tx_bufs.buffers = (const struct spi_buf *)&txb[0];
		tx_bufs.count = 1U;

		rx_bufs.buffers = NULL;
		rx_bufs.count = 0U;

		ret = spi_transceive(spi_dev,
				(const struct spi_config *)&spi_cfg_single,
				(const struct spi_buf_set *)&tx_bufs,
				(const struct spi_buf_set *)&rx_bufs);
		zassert_true(ret == 0,
				"Write spi status2 QE=1 spi_transceive failure: "
				"error %d", ret);

		/* read register2 to confirm quad mode is enabled */
		safbuf[0] = SPI_READ_STATUS2;
		txb[0].buf = &safbuf;
		txb[0].len = 1U;

		tx_bufs.buffers = (const struct spi_buf *)&txb[0];
		tx_bufs.count = 1U;

		memset(safbuf2, 0, 1U);
		rxb.buf = &safbuf2;
		rxb.len = 1U;

		rx_bufs.buffers = (const struct spi_buf *)&rxb;
		rx_bufs.count = 1U;

		ret = spi_transceive(spi_dev,
				(const struct spi_config *)&spi_cfg_single,
				(const struct spi_buf_set *)&tx_bufs,
				(const struct spi_buf_set *)&rx_bufs);
		zassert_true(ret == 0, "Read register2 status spi_transceive failure: "
				"error %d", ret);

		spi_status2 = safbuf2[0];
		zassert_true((spi_status2 & SPI_STATUS2_QE) == SPI_STATUS2_QE,
				"Enable QSPI mode failure");
	}

	/* write enable */
	safbuf[0] = SPI_WRITE_ENABLE;
	txb[0].buf = &safbuf;
	txb[0].len = 1U;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	rx_bufs.buffers = NULL;
	rx_bufs.count = 0U;

	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Send write enable spi_transceive failure: "
			"error %d", ret);

	/* configure spi quad mode */
	spi_cfg_quad.frequency = TEST_FREQ_HZ;
	spi_cfg_quad.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB
		| SPI_WORD_SET(8) | SPI_LINES_QUAD;
	spi_cfg_quad.slave = 0;
	spi_cfg_quad.cs = NULL;

	/* write data using spi quad mode */
	/* send quad write opcode and address using single mode */
	memset(safbuf, 0, TEST_BUF_SIZE);
	safbuf[0] = SPI_QUAD_WRITE_DATA;
	safbuf[1] = SPI_TEST_ADDRESS_2 & 0xFFFFFFU;

	txb[0].buf = &safbuf;
	txb[0].len = 4;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	rx_bufs.buffers = NULL;
	rx_bufs.count = 0U;

	spi_cfg_single.operation |= SPI_HOLD_ON_CS;
	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Send quad write data spi_transceive failure: "
			"error %d", ret);

	/* send data using quad mode */
	memset(safbuf, 0, TEST_BUF_SIZE);
	memcpy(&safbuf[0], buffer_tx_2, BUF_SIZE_2);

	txb[0].buf = &safbuf;
	txb[0].len = BUF_SIZE_2;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	rx_bufs.buffers = NULL;
	rx_bufs.count = 0U;

	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_quad,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Send quad write data spi_transceive failure: "
			"error %d", ret);

	spi_cfg_single.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB
		| SPI_WORD_SET(8) | SPI_LINES_SINGLE;
	ret = spi_release(spi_dev, (const struct spi_config *)&spi_cfg_single);
	zassert_true(ret == 0, "Spi release failure: error %d", ret);
}

/**
 * @brief Read data from flash using spi quad mode
 * @details
 * - read data using spi quad mode
 * - check read buffer data whether correct
 */
ZTEST_USER(spi_quad, test_spi_quad_read)
{
	int ret;
	uint8_t cnt = 0;
	uint16_t spi_opcode;

	spi_opcode = SPI_QUAD_FAST_READ_DATA;

	/* read data using spi quad mode */
	/* set the spi operation code and address */
	memset(safbuf, 0, TEST_BUF_SIZE);
	safbuf[0] = spi_opcode & 0xFFU;
	safbuf[1] = SPI_TEST_ADDRESS_2 & 0xFFFFFFU;

	txb[cnt].buf = &safbuf;
	txb[cnt].len = 4U;

	/* set the dummy clocks */
	if (spi_opcode & 0xFF00U) {
		cnt++;
		txb[cnt].buf = NULL;
		txb[cnt].len = (spi_opcode >> 8) & 0xFFU;
	}

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = cnt + 1;

	/* send opcode and address using single mode */
	spi_cfg_single.operation |= SPI_HOLD_ON_CS;
	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs, NULL);
	zassert_true(ret == 0, "Send fast read data spi_transceive failure: "
			"error %d", ret);

	memset(safbuf2, 0, TEST_BUF_SIZE);
	rxb.buf = &safbuf2;
	rxb.len = BUF_SIZE_2;

	rx_bufs.buffers = (const struct spi_buf *)&rxb;
	rx_bufs.count = 1U;

	/* get read data using quad mode */
	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_quad,
			NULL, (const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Receive fast read data spi_transceive failure: "
			"error %d", ret);

	/* check read buffer data whether correct */
	zassert_true(memcmp(buffer_tx_2, safbuf2, BUF_SIZE_2) == 0,
			"Buffer read data is different to write data");

	/* release spi device */
	spi_cfg_single.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB
		| SPI_WORD_SET(8) | SPI_LINES_SINGLE;
	ret = spi_release(spi_dev, (const struct spi_config *)&spi_cfg_single);
	zassert_true(ret == 0, "Spi release failure: error %d", ret);
}

/**
 * @brief Read data from flash using spi octal quad mode
 * @details
 * - read data using spi octal quad mode
 * - check read buffer data whether correct
 */
ZTEST_USER(spi_quad, test_spi_octal_read)
{
	int ret;

	/* read data using spi octal quad mode */
	/* send octal read opcode using single mode */
	memset(safbuf, 0, TEST_BUF_SIZE);
	safbuf[0] = SPI_OCTAL_QUAD_READ_DATA;

	txb[0].buf = &safbuf;
	txb[0].len = 1;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	rx_bufs.buffers = NULL;
	rx_bufs.count = 0U;

	spi_cfg_single.operation |= SPI_HOLD_ON_CS;
	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_single,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Send octal quad read opcode "
			"spi_transceive failure: error %d", ret);

	/* send address using quad mode */
	safbuf[0] = SPI_TEST_ADDRESS & 0xFFFFF0U;
	safbuf[3] = 0xFFU;
	txb[0].buf = &safbuf;
	txb[0].len = 4;

	tx_bufs.buffers = (const struct spi_buf *)&txb[0];
	tx_bufs.count = 1U;

	memset(safbuf2, 0, TEST_BUF_SIZE);
	rxb.buf = &safbuf2;
	rxb.len = BUF_SIZE;

	rx_bufs.buffers = (const struct spi_buf *)&rxb;
	rx_bufs.count = 1U;

	ret = spi_transceive(spi_dev,
			(const struct spi_config *)&spi_cfg_quad,
			(const struct spi_buf_set *)&tx_bufs,
			(const struct spi_buf_set *)&rx_bufs);
	zassert_true(ret == 0, "Send quad read address spi_transceive "
			"failure: error %d", ret);

	/* check read buffer data whether correct */
	zassert_true(memcmp(buffer_tx, safbuf2, BUF_SIZE) == 0,
			"Buffer read data is different to write data");

	/* release spi device */
	spi_cfg_single.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB
		| SPI_WORD_SET(8) | SPI_LINES_SINGLE;
	ret = spi_release(spi_dev, (const struct spi_config *)&spi_cfg_single);
	zassert_true(ret == 0, "Spi release failure: error %d", ret);
}


void *spi_setup(void)
{
	spi_single_init();

	return NULL;
}

void *spi_single_setup(void)
{
	spi_single_init();

	/* The writing test goes
	 * first berfore testing
	 * the reading.
	 */
	test_spi_single_write();

	return NULL;
}

void *spi_quad_setup(void)
{
	spi_dual_init();

	/* The writing test goes
	 * first berfore testing
	 * the reading.
	 */
	test_spi_quad_write();

	return NULL;
}

ZTEST_SUITE(spi, NULL, spi_single_setup, NULL, NULL, NULL);
ZTEST_SUITE(spi_quad, NULL, spi_quad_setup, NULL, NULL, NULL);
ZTEST_SUITE(spi_sector_erase, NULL, spi_setup, NULL, NULL, NULL);
