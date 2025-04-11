/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_APS6404L_H_
#define ZEPHYR_DRIVERS_SPI_APS6404L_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

/* APS6404L communication commands */
#define APS6404L_WRITE_REG  0x0A
#define APS6404L_READ_REG   0x0B
#define APS6404L_WRITE_FIFO 0x0D

#define APS6404L_WRITE            0x02
#define APS6404L_READ             0x03
#define APS6404L_FAST_READ        0x0B
#define APS6404L_QUAD_MODE_ENTER  0x35
#define APS6404L_QUAD_WRITE       0x38
#define APS6404L_RESET_ENABLE     0x66
#define APS6404L_RESET_MEMORY     0x99
#define APS6404L_READ_ID          0x9F
#define APS6404L_HALF_SLEEP_ENTER 0xC0
#define APS6404L_QUAD_READ        0xEB
#define APS6404L_QUAD_MODE_EXIT   0xF5

#define APS6404L_LOCAL_MAX_SIZE 256
/* Page size - limits the burst write/read */
#define APS6404L_PAGE_SIZE      1024

#define APS6404L_PART_ID 0x5D0D

#define APS6404L_SPEED_48MHZ 48000000
#define APS6404L_SPEED_24MHZ 24000000
#define APS6404L_SPEED_16MHZ 16000000
#define APS6404L_SPEED_12MHZ 12000000
#define APS6404L_SPEED_8MHZ  8000000

#define APS6404L_48MHZ_MAX_BYTES 32
#define APS6404L_24MHZ_MAX_BYTES 16
#define APS6404L_16MHZ_MAX_BYTES 10
#define APS6404L_12MHZ_MAX_BYTES 6
#define APS6404L_8MHZ_MAX_BYTES  3

struct aps6404l_config {
	struct spi_dt_spec spec;
};

struct aps6404l_data {
	uint32_t ui32Data;
};

extern int aps6404l_write(const struct device *dev, uint8_t *pui8TxBuffer,
			  uint32_t ui32WriteAddress, uint32_t ui32NumBytes);

extern int aps6404l_read(const struct device *dev, uint8_t *pui8RxBuffer, uint32_t ui32ReadAddress,
			 uint32_t ui32NumBytes);
#endif /* ZEPHYR_DRIVERS_SPI_APS6404L_H_ */
