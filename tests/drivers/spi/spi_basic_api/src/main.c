/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_spi
 * @{
 * @defgroup t_spi_basic test_spi_basic_operations
 * @}
 *
 * Setup: Connect DUT MISO to DUT MOSI
 *
 * quark_se_c1000_devboard - x86
 * ----------------------
 *
 * SPI_0_MISO - SPI_0_MOSI
 * SPI_1_MISO - SPI_1_MOSI
 *
 * quark_se_c1000_ss_devboard - arc
 * ----------------------
 *
 * SPI_SS_0_MISO - SPI_SS_0_MOSI
 * SPI_SS_1_MISO - SPI_SS_1_MOSI
 *
 * quark_d2000 - x86
 * ----------------------
 *
 * SPI_M_MISO - SPI_M_MOSI
 *
 * arduino_101 - x86
 * ----------------------
 *
 * SPI1_M_MISO - SPI1_M_MOSI
 */

#include <zephyr.h>
#include <ztest.h>

void test_spi_cpol(void);
void test_spi_cpha(void);
void test_spi_cpol_cpha(void);

void test_main(void)
{
	ztest_test_suite(spi_test,
			 ztest_unit_test(test_spi_cpol),
			 ztest_unit_test(test_spi_cpha),
			 ztest_unit_test(test_spi_cpol_cpha));
	ztest_run_test_suite(spi_test);
}
