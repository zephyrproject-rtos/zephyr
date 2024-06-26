/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_FRAMMB85RS64V_FRAMMB85RS64V_H_
#define ZEPHYR_DRIVERS_SENSOR_FRAMMB85RS64V_FRAMMB85RS64V_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>

#define DT_DRV_COMPAT fujitsu_frammb85rs64v

#define FRAMMB85RS64V_BUS_SPI DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

int frammb85rs64v_fram_read(const struct device *dev, uint16_t addr, uint8_t *data, size_t len);
int frammb85rs64v_fram_write(const struct device *dev, uint16_t addr, const uint8_t *data,
			     size_t len);
union frammb85rs64v_bus {
#if FRAMMB85RS64V_BUS_SPI
	struct spi_dt_spec spi;
#endif
};

typedef int (*frammb85rs64v_bus_check_fn)(const union frammb85rs64v_bus *bus);
typedef int (*frammb85rs64v_reg_read_fn)(const union frammb85rs64v_bus *bus, uint16_t addr,
					 uint8_t *data, size_t len);
typedef int (*frammb85rs64v_reg_write_fn)(const union frammb85rs64v_bus *bus, uint16_t addr,
					  const uint8_t *data, size_t len);
typedef int (*frammb85rs64v_read_id_fn)(const union frammb85rs64v_bus *bus, uint8_t *data);
struct frammb85rs64v_bus_io {
	frammb85rs64v_bus_check_fn check;
	frammb85rs64v_reg_read_fn read;
	frammb85rs64v_reg_write_fn write;
	frammb85rs64v_read_id_fn read_id;
};

#if FRAMMB85RS64V_BUS_SPI
#define FRAMMB85RS64V_SPI_OPERATION                                                                \
	(SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPOL | SPI_MODE_CPHA)
extern const struct frammb85rs64v_bus_io frammb85rs64v_bus_io_spi;
#endif

#define MB85RS64V_MANUFACTURER_ID_CMD 0x9f
#define MB85RS64V_WRITE_ENABLE_CMD 0x06
#define MB85RS64V_READ_CMD 0x03
#define MB85RS64V_WRITE_CMD 0x02
#define MAX_USER_DATA_LENGTH 1024
#define FRAMMB85RS64V_CHIP_ID 0x047f0302

#define FRAMMB85RS64V_MODE_NORMAL 0x03

#if defined CONFIG_FRAMMB85RS64V_MODE_NORMAL
#define FRAMMB85RS64V_MODE FRAMMB85RS64V_MODE_NORMAL
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_FRAMMB85RS64V_FRAMMB85RS64V_H_ */
