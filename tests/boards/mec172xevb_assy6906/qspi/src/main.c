/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/spi.h>

#define TEST_FREQ_HZ     24000000U
#define W25Q128_JEDEC_ID 0x001840efU

#define TEST_BUF_SIZE 4096U
/* #define MAX_SPI_BUF 8 */

#define SPI_STATUS1_BUSY      0x01U
#define SPI_STATUS1_WEL       0x02U
#define SPI_STATUS2_QE        0x02U
#define SPI_READ_JEDEC_ID     0x9FU
#define SPI_READ_STATUS1      0x05U
#define SPI_READ_STATUS2      0x35U
#define SPI_WRITE_STATUS2     0x31U
#define SPI_WRITE_ENABLE_VS   0x50U
#define SPI_WRITE_ENABLE      0x06U
#define SPI_SECTOR_ERASE      0x20U
#define SPI_SINGLE_WRITE_DATA 0x02U
#define SPI_QUAD_WRITE_DATA   0x32U

/* bits[7:0] = spi opcode,
 * bits[15:8] = bytes number of clocks with data lines tri-stated
 */
#define SPI_FAST_READ_DATA 0x080BU

#define BUF_SIZE 11
uint8_t buffer_tx[] = "0123456789\0";
#define BUF_SIZE_2 7
uint8_t buffer_tx_2[] = "abcdef\0";

#define SPI_TEST_ADDRESS   0x000010U
#define SPI_TEST_ADDRESS_2 0x000020U

static uint8_t safbuf[TEST_BUF_SIZE] __aligned(4);
static uint8_t safbuf2[TEST_BUF_SIZE] __aligned(4);
static const struct device *const spi_dev = DEVICE_DT_GET(DT_NODELABEL(qspi0));

static const struct spi_config spi_cfg_single = {
	.frequency = TEST_FREQ_HZ,
	.operation = (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_LINES_SINGLE),
};

static void spi_single_init(void)
{
	zassert_true(device_is_ready(spi_dev), "SPI controller device is not ready");
}

static void clear_buffers(void)
{
	memset(safbuf, 0, sizeof(safbuf));
	memset(safbuf2, 0, sizeof(safbuf2));
}

/* Compute the number of bytes required to generate the requested number of
 * SPI clocks based on single, dual, or quad mode.
 * mode = 1(full-duplex), 2(dual), 4(quad)
 * full-duplex: 8 clocks per byte
 * dual: 4 clocks per byte
 * quad: 2 clocks per byte
 */
static uint32_t spi_clocks_to_bytes(uint32_t spi_clocks, uint8_t mode)
{
	uint32_t nbytes;

	if (mode == 4u) {
		nbytes = spi_clocks / 2U;
	} else if (mode == 2u) {
		nbytes = spi_clocks / 4U;
	} else {
		nbytes = spi_clocks / 8U;
	}

	return nbytes;
}

static int spi_flash_address_format(uint8_t *dest, size_t destsz, uint32_t spi_addr, size_t addrsz)
{
	if (!dest || (addrsz == 0) || (addrsz > 4U) || (addrsz > destsz)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < addrsz; i++) {
		dest[i] = (uint8_t)((spi_addr >> ((addrsz - (i + 1U)) * 8U)) & 0xffU);
	}

	return 0;
}

static int spi_flash_read_status(const struct device *dev, uint8_t opcode, uint8_t *status)
{
	struct spi_buf spi_bufs[2] = {0};
	uint32_t txdata = 0;
	uint32_t rxdata = 0;
	int ret = 0;

	txdata = opcode;

	spi_bufs[0].buf = &txdata;
	spi_bufs[0].len = 1U;
	spi_bufs[1].buf = &rxdata;
	spi_bufs[1].len = 1U;

	const struct spi_buf_set txset = {
		.buffers = &spi_bufs[0],
		.count = 2U,
	};
	const struct spi_buf_set rxset = {
		.buffers = &spi_bufs[0],
		.count = 2U,
	};

	ret = spi_transceive(spi_dev, &spi_cfg_single, &txset, &rxset);
	if (ret) {
		return ret;
	}

	if (status) {
		*status = (uint8_t)(rxdata & 0xffu);
	}

	return 0;
}

static int spi_flash_tx_one_byte_cmd(const struct device *dev, uint8_t opcode)
{
	struct spi_buf spi_bufs[1] = {0};
	uint32_t txdata = 0;
	int ret = 0;

	txdata = opcode;
	spi_bufs[0].buf = &txdata;
	spi_bufs[0].len = 1U;

	const struct spi_buf_set txset = {
		.buffers = &spi_bufs[0],
		.count = 1U,
	};

	ret = spi_transceive(spi_dev, &spi_cfg_single, &txset, NULL);
	if (ret) {
		return ret;
	}

	return 0;
}

/**
 * @brief Test spi device
 * @details
 * - Find spi device
 * - Read flash jedec id
 */
ZTEST_USER(spi, test_spi_device)
{
	struct spi_buf spi_bufs[2] = {0};
	uint32_t txdata = 0;
	uint32_t jedec_id = 0;
	int ret = 0;

	/* read jedec id */
	txdata = SPI_READ_JEDEC_ID;

	spi_bufs[0].buf = &txdata;
	spi_bufs[0].len = 1U;
	spi_bufs[1].buf = &jedec_id;
	spi_bufs[1].len = 3U;

	const struct spi_buf_set txset = {
		.buffers = &spi_bufs[0],
		.count = 2U,
	};
	const struct spi_buf_set rxset = {
		.buffers = &spi_bufs[0],
		.count = 2U,
	};

	ret = spi_transceive(spi_dev, &spi_cfg_single, &txset, &rxset);
	zassert_true(ret == 0,
		     "Read JEDEC ID spi_transceive failure: "
		     "error %d",
		     ret);
	zassert_true(jedec_id == W25Q128_JEDEC_ID,
		     "JEDEC ID doesn't match: expected 0x%08x, read 0x%08x", W25Q128_JEDEC_ID,
		     jedec_id);
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
	struct spi_buf spi_bufs[2] = {0};
	int ret = 0;
	uint8_t spi_status = 0;

	clear_buffers();

	/* write enable */
	ret = spi_flash_tx_one_byte_cmd(spi_dev, SPI_WRITE_ENABLE);
	zassert_true(ret == 0, "Send write enable spi_transceive failure: error %d", ret);

	/* erase data start from address SPI_TEST_ADDRESS */
	safbuf[0] = SPI_SECTOR_ERASE;
	spi_flash_address_format(&safbuf[1], 4U, SPI_TEST_ADDRESS, 3U);

	spi_bufs[0].buf = &safbuf;
	spi_bufs[0].len = 4;

	const struct spi_buf_set txset = {
		.buffers = &spi_bufs[0],
		.count = 1U,
	};

	ret = spi_transceive(spi_dev, &spi_cfg_single, &txset, NULL);
	zassert_true(ret == 0, "Send sector erase data spi_transceive failure: error %d", ret);

	/* read SPI flash status  register1 to check whether erase operation completed */
	spi_status = SPI_STATUS1_BUSY;
	while (spi_status & SPI_STATUS1_BUSY) {
		ret = spi_flash_read_status(spi_dev, SPI_READ_STATUS1, &spi_status);
		zassert_true(ret == 0,
			     "Send read register1 spi_transceive "
			     "failure: error %d",
			     ret);
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
	struct spi_buf spi_bufs[1] = {0};
	int ret = 0;
	uint8_t spi_status = 0;

	clear_buffers();

	ret = spi_flash_tx_one_byte_cmd(spi_dev, SPI_WRITE_ENABLE);
	zassert_true(ret == 0,
		     "Send write enable spi_transceive failure: "
		     "error %d",
		     ret);

	/* write data start from address SPI_TEST_ADDRESS */
	safbuf[0] = SPI_SINGLE_WRITE_DATA;
	spi_flash_address_format(&safbuf[1], 4U, SPI_TEST_ADDRESS, 3U);

	memcpy(&safbuf[4], buffer_tx, BUF_SIZE);

	spi_bufs[0].buf = &safbuf;
	spi_bufs[0].len = 4U + BUF_SIZE;

	const struct spi_buf_set txset = {
		.buffers = &spi_bufs[0],
		.count = 1U,
	};

	ret = spi_transceive(spi_dev, &spi_cfg_single, &txset, NULL);
	zassert_true(ret == 0, "Send write data spi_transceive failure: error %d", ret);

	/* read register1 to check whether program operation completed */
	spi_status = SPI_STATUS1_BUSY;
	while (spi_status & SPI_STATUS1_BUSY) {
		ret = spi_flash_read_status(spi_dev, SPI_READ_STATUS1, &spi_status);
		zassert_true(ret == 0, "Read SPI flash STATUS opcode 0x%02x error: %d",
			     SPI_READ_STATUS1, ret);
	}
}

/**
 * @brief Read data from flash using spi single mode
 * @details
 * - read data using spi single mode
 * - check read buffer data whether correct
 * @note SPI flash fast instructions require a certain number of SPI clocks
 * to be generated with I/O lines tri-stated after the address has been
 * transmitted. The purpose is allow SPI flash time to move get data ready
 * and enable its output line(s). The MCHP XEC SPI driver can do this by
 * specifying a struct spi_buf with buf pointer set to NULL and length set
 * to the number of bytes which will generate the required number of clocks.
 * For full-duplex one byte is 8 clocks, dual one byte is 4 clocks, and for
 * quad one byte is 2 clocks.
 */
ZTEST_USER(spi, test_spi_single_read)
{
	struct spi_buf spi_bufs[3] = {0};
	int ret = 0;
	uint16_t spi_opcode = 0;
	uint8_t cnt = 0;

	clear_buffers();

	/* bits[7:0] = opcode,
	 * bits[15:8] = number of SPI clocks with I/O lines tri-stated after
	 * address transmit before data read phase.
	 */
	spi_opcode = SPI_FAST_READ_DATA;

	/* read data using spi single mode */
	/* set the spi operation code and address */
	safbuf[0] = spi_opcode & 0xFFU;
	spi_flash_address_format(&safbuf[1], 4U, SPI_TEST_ADDRESS, 3U);

	spi_bufs[cnt].buf = &safbuf;
	spi_bufs[cnt].len = 4U;

	/* set the dummy clocks */
	if (spi_opcode & 0xFF00U) {
		cnt++;
		spi_bufs[cnt].buf = NULL;
		spi_bufs[cnt].len = spi_clocks_to_bytes(((spi_opcode >> 8) & 0xffU), 1u);
	}

	cnt++;
	spi_bufs[cnt].buf = &safbuf2;
	spi_bufs[cnt].len = BUF_SIZE;
	cnt++; /* total number of buffers */

	const struct spi_buf_set txset = {
		.buffers = &spi_bufs[0],
		.count = cnt,
	};

	const struct spi_buf_set rxset = {
		.buffers = &spi_bufs[0],
		.count = cnt,
	};

	ret = spi_transceive(spi_dev, &spi_cfg_single, &txset, &rxset);
	zassert_true(ret == 0, "Send fast read data spi_transceive failure: error %d", ret);

	/* check read buffer data whether correct */
	zassert_true(memcmp(buffer_tx, safbuf2, BUF_SIZE) == 0,
		     "Buffer read data is different to write data");
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
	 * first before testing
	 * the reading.
	 */
	test_spi_single_write();

	return NULL;
}

/* Test assumes flash test regions is in erased state */
ZTEST_SUITE(spi, NULL, spi_single_setup, NULL, NULL, NULL);
ZTEST_SUITE(spi_sector_erase, NULL, spi_setup, NULL, NULL, NULL);
