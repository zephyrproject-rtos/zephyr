/*
 * Copyright (c) 2025 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_serial_flash, CONFIG_SPI_LOG_LEVEL);

#define DT_DRV_COMPAT apm_aps6404l

#define TEST_ADDRESS 0x10

/*
 * A build error on these lines means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
const struct device *aps6404l_dev = DEVICE_DT_GET(DT_ALIAS(spi_psram));

#define TRANSACTION_SIZE 128

#define MAX_TRANS_SIZE (TRANSACTION_SIZE)
static uint8_t g_pucInBuffer[TRANSACTION_SIZE * 2];
static uint8_t g_pucOutBuffer[TRANSACTION_SIZE * 2];

static int load_buffers(int offset, int num_addr, int repeatCount)
{
	/* Returns number of bytes loaded */
	int ix = 0;

	/* Initialize the buffer section which will be used - to initialize out and in different */
	for (ix = offset; ix < num_addr + offset; ix++) {
		g_pucOutBuffer[ix] = (ix - offset) & 0xFF;
		g_pucInBuffer[ix] = ~g_pucOutBuffer[ix];
	}

	return ix;
}

static bool compare_buffers(uint32_t offset, int num_addr, int repeatCount)
{
	/* Compare the output buffer with input buffer */
	for (uint32_t i = offset; i < offset + num_addr; i++) {
		if (g_pucInBuffer[i] != g_pucOutBuffer[i]) {
			LOG_ERR("Buffer miscompare at location %d\n", i);
			LOG_ERR("TX Value = %2X | RX Value = %2X\n", g_pucOutBuffer[i],
				g_pucInBuffer[i]);
			LOG_ERR("TX:\n");
			for (uint32_t k = 0; k < num_addr; k++) {
				LOG_ERR("%2X ", g_pucOutBuffer[offset + k]);
				if ((k % 64) == 0) {
					LOG_ERR("\n");
				}
			}
			LOG_ERR("\n");
			LOG_ERR("RX:\n");
			for (uint32_t k = 0; k < num_addr; k++) {
				LOG_ERR("%2X ", g_pucInBuffer[offset + k]);
				if ((k % 64) == 0) {
					LOG_ERR("\n");
				}
			}
			LOG_ERR("\n");
			return false;
		}
	}
	return true;
}

extern int aps6404l_write(const struct device *dev, uint8_t *pui8TxBuffer,
			  uint32_t ui32WriteAddress, uint32_t ui32NumBytes);
extern int aps6404l_read(const struct device *dev, uint8_t *pui8RxBuffer, uint32_t ui32ReadAddress,
			 uint32_t ui32NumBytes);

void erase_address_space(const struct device *dev)
{
	/* Set the buffer to the PSRAM device. */
	int ret = aps6404l_write(aps6404l_dev, &g_pucOutBuffer[0], TEST_ADDRESS, TRANSACTION_SIZE);

	if (ret == 0) {
		printk("\nAPS6404L Erase Write PASSED\n");

		/* Read the buffer from the PSRAM device. */
		ret = aps6404l_read(aps6404l_dev, &g_pucInBuffer[0], TEST_ADDRESS,
				    TRANSACTION_SIZE);
	} else {
		LOG_ERR("APS6404L EraseWrite FAILED\n");
	}

	/* Compare the receive buffer to the transmit buffer. */
	if (ret || !compare_buffers(0, TRANSACTION_SIZE, 0)) {
		LOG_ERR("APS6404L Erase Read or compare_buffers FAILED\n");
	} else {
		printk("APS6404L Erase Read PASSED\n");
	}
}

int main(void)
{
	int ret = 0;

	if (!device_is_ready(aps6404l_dev)) {
		return -1;
	}

	memset(g_pucOutBuffer, 0xFF, TRANSACTION_SIZE);
	erase_address_space(aps6404l_dev);

	/* Create the transmit buffer to write. */
	load_buffers(0, TRANSACTION_SIZE, 0);

	/* Write the buffer to the PSRAM device. */
	ret = aps6404l_write(aps6404l_dev, g_pucOutBuffer, TEST_ADDRESS, TRANSACTION_SIZE);

	if (ret == 0) {
		printk("\nAPS6404L Write PASSED\n");

		/* Read the buffer from the PSRAM device. */
		ret = aps6404l_read(aps6404l_dev, g_pucInBuffer, TEST_ADDRESS, TRANSACTION_SIZE);
	} else {
		LOG_ERR("APS6404L Write FAILED\n");
	}

	/* Compare the receive buffer to the transmit buffer. */
	if (ret || !compare_buffers(0, TRANSACTION_SIZE, 0)) {
		LOG_ERR("APS6404L Read or compare_buffers FAILED\n");
	} else {
		printk("APS6404L Read PASSED\n");
	}

	return 0;
}
