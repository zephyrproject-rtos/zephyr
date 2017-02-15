/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_spi_basic
 * @{
 * @defgroup t_spi_basic_operations test_spi_basic_operation_mode
 * @brief TestPurpose: verify SPI basic operations in different mode
 * @}
 */

#include <spi.h>
#include <zephyr.h>
#include <ztest.h>

#ifdef CONFIG_ARC
#define SPI_DEV_NAME CONFIG_SPI_SS_0_NAME
#else
#ifdef CONFIG_BOARD_ARDUINO_101
#define SPI_DEV_NAME CONFIG_SPI_1_NAME
#else
#define SPI_DEV_NAME CONFIG_SPI_0_NAME
#endif
#endif

#define SPI_SLAVE 1
#define SPI_MAX_CLK_FREQ_250KHZ 128

static struct spi_config spi_conf = {
	.config = SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD(8),
	.max_sys_freq = SPI_MAX_CLK_FREQ_250KHZ,
};

static unsigned char wbuf[16] = "Hello";
static unsigned char rbuf[16] = {};

static int test_spi(uint32_t mode)
{
	struct device *spi_dev = device_get_binding(SPI_DEV_NAME);
	uint32_t len = strlen(wbuf);

	if (!spi_dev) {
		TC_PRINT("Cannot get SPI device\n");
		return TC_FAIL;
	}

	spi_conf.config = mode | SPI_MODE_LOOP;
	/* 1. verify spi_configure() */
	if (spi_configure(spi_dev, &spi_conf)) {
		TC_PRINT("SPI config failed\n");
		return TC_FAIL;
	}

	/* 2. verify spi_slave_select() */
	if (spi_slave_select(spi_dev, SPI_SLAVE)) {
		TC_PRINT("SPI slave select failed\n");
		return TC_FAIL;
	}

	/* 3. verify spi_write() */
	if (spi_write(spi_dev, (uint8_t *) wbuf, 6) != 0) {
		TC_PRINT("SPI write failed\n");
		return TC_FAIL;
	}

	strcpy((char *)wbuf, "So what then?");
	len = strlen(wbuf);

	/* 4. verify spi_transceive() */
	TC_PRINT("SPI sent: %s\n", wbuf);
	if (spi_transceive(spi_dev, wbuf, len + 1, rbuf, len + 1) != 0) {
		TC_PRINT("SPI transceive failed\n");
		return TC_FAIL;
	}

	TC_PRINT("SPI transceived: %s\n", rbuf);

	if (!strcmp(wbuf, rbuf)) {
		return TC_PASS;
	} else {
		return TC_FAIL;
	}
}

void test_spi_cpol(void)
{
	TC_PRINT("Test SPI_MODE_CPOL\n");
	assert_true(test_spi(SPI_WORD(8) | SPI_MODE_CPOL) == TC_PASS, NULL);
}

void test_spi_cpha(void)
{
	TC_PRINT("Test SPI_MODE_CPHA\n");
	assert_true(test_spi(SPI_WORD(8) | SPI_MODE_CPHA) == TC_PASS, NULL);
}

void test_spi_cpol_cpha(void)
{
	TC_PRINT("Test SPI_MODE_CPOL | SPI_MODE_CPHA\n");
	assert_true(test_spi(SPI_WORD(8) | SPI_MODE_CPOL | SPI_MODE_CPHA)
					== TC_PASS, NULL);
}
