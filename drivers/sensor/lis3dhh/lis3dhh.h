/*
 * Copyright (c) Nexplore Technology GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS3DHH_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS3DHH_H_

#include <kernel.h>
#include <device.h>
#include <sys/util.h>
#include <stdint.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <string.h>
#include <drivers/spi.h>
#include <zephyr/types.h>
#include <devicetree.h>

#define LIS3DHH_REG_WHO_AM_I 0x0F
#define LIS3DHH_CHIP_ID 0x11

#define LIS3DHH_CTRL_REG1 0x20
#define LIS3DHH_CTRL_REG1_NORM_MODE_EN BIT(7)
#define LIS3DHH_CTRL_REG1_IF_ADD_INC BIT(6)
#define LIS3DHH_CTRL_REG1_ZERO_BIT_2 BIT(5)
#define LIS3DHH_CTRL_REG1_ZERO_BIT_1 BIT(4)
#define LIS3DHH_CTRL_REG1_BOOT BIT(3)
#define LIS3DHH_CTRL_REG1_SW_RESET BIT(2)
#define LIS3DHH_CTRL_REG1_DRDY_PULSE BIT(1)
#define LIS3DHH_CTRL_REG1_BDU BIT(0)

#define LIS3DHH_INT1_CTRL 0x21
#define LIS3DHH_INT1_CTRL_DRDY BIT(7)
#define LIS3DHH_INT1_CTRL_BOOT BIT(6)
#define LIS3DHH_INT1_CTRL_OVR BIT(5)
#define LIS3DHH_INT1_CTRL_FSS5 BIT(4)
#define LIS3DHH_INT1_CTRL_FTH BIT(3)
#define LIS3DHH_INT1_CTRL_EXT BIT(2)
#define LIS3DHH_INT1_CTRL_ZERO_2 BIT(1)
#define LIS3DHH_INT1_CTRL_ZERO_1 BIT(0)

#define LIS3DHH_INT2_CTRL 0x22
#define LIS3DHH_INT2_CTRL_INT2DEDY BIT(7)
#define LIS3DHH_INT2_CTRL_BOOT BIT(6)
#define LIS3DHH_INT2_CTRL_OVR BIT(5)
#define LIS3DHH_INT2_CTRL_FSS5 BIT(4)
#define LIS3DHH_INT2_CTRL_FTH BIT(3)
#define LIS3DHH_INT2_CTRL_ZERO_3 BIT(2)
#define LIS3DHH_INT2_CTRL_ZERO_2 BIT(1)
#define LIS3DHH_INT2_CTRL_ZERO_1 BIT(0)

#define LIS3DHH_CTRL_REG4 0x23
#define LIS3DHH_CTRL_REG4_DSP_LP_TYPE BIT(7)
#define LIS3DHH_CTRL_REG4_DSP_BW_SEL BIT(6)
#define LIS3DHH_CTRL_REG4_ST2 BIT(5)
#define LIS3DHH_CTRL_REG4_ST1 BIT(4)
#define LIS3DHH_CTRL_REG4_PP_OD_INT2 BIT(3)
#define LIS3DHH_CTRL_REG4_PP_OD_INT1 BIT(2)
#define LIS3DHH_CTRL_REG4_FIFO_EN BIT(1)
#define LIS3DHH_CTRL_REG4_ONE_1 BIT(0)

#define LIS3DHH_CTRL_REG5 0x24
#define LIS3DHH_CTRL_REG5_ZERO_7 BIT(7)
#define LIS3DHH_CTRL_REG5_ZERO_6 BIT(6)
#define LIS3DHH_CTRL_REG5_ZERO_5 BIT(5)
#define LIS3DHH_CTRL_REG5_ZERO_4 BIT(4)
#define LIS3DHH_CTRL_REG5_ZERO_3 BIT(3)
#define LIS3DHH_CTRL_REG5_ZERO_2 BIT(2)
#define LIS3DHH_CTRL_REG5_ZERO_1 BIT(1)
#define LIS3DHH_CTRL_REG5_FIFO_SPI_HS_ON BIT(0)

#define LIS3DHH_OUT_TEMP_L 0x25 /* left-justified, BIT(0) to BIT(3) are 0 */
#define LIS3DHH_OUT_TEMP_H 0x26 /* left-justified */

#define LIS3DHH_STATUS 0x27
#define LIS3DHH_STATUS_ZYX_OVR BIT(7)
#define LIS3DHH_STATUS_Z_OVR BIT(6)
#define LIS3DHH_STATUS_Y_OVR BIT(5)
#define LIS3DHH_STATUS_X_OVR BIT(4)
#define LIS3DHH_STATUS_OVR_MASK (BIT_MASK(4) << 4)
#define LIS3DHH_STATUS_ZYX_DRDY BIT(3)
#define LIS3DHH_STATUS_Z_DRDY BIT(2)
#define LIS3DHH_STATUS_Y_DRDY BIT(1)
#define LIS3DHH_STATUS_X_DRDY BIT(0)
#define LIS3DHH_STATUS_DRDY_MASK BIT_MASK(4)

/*
 * Linear acceleration sensor XYZ-axis output registers.
 * The value is expressed as a 16-bit word in two's complement,
 * left-justified
 */
#define LIS3DHH_REG_ACCEL_X_LSB 0x28
#define LIS3DHH_REG_ACCEL_X_MSB 0x29
#define LIS3DHH_REG_ACCEL_Y_LSB 0x2A
#define LIS3DHH_REG_ACCEL_Y_MSB 0x2B
#define LIS3DHH_REG_ACCEL_Z_LSB 0x2C
#define LIS3DHH_REG_ACCEL_Z_MSB 0x2D

#define LIS3DHH_FIFO_CTRL 0x2E
#define LIS3DHH_FIFO_CTRL_FMODE2 BIT(7)
#define LIS3DHH_FIFO_CTRL_FMODE1 BIT(6)
#define LIS3DHH_FIFO_CTRL_FMODE0 BIT(5)
#define LIS3DHH_FIFO_CTRL_FTH4 BIT(4)
#define LIS3DHH_FIFO_CTRL_FTH3 BIT(3)
#define LIS3DHH_FIFO_CTRL_FTH2 BIT(2)
#define LIS3DHH_FIFO_CTRL_FTH1 BIT(1)
#define LIS3DHH_FIFO_CTRL_FTH0 BIT(0)

#define LIS3DHH_FIFO_SRC 0x2F
#define LIS3DHH_FIFO_SRC_FTH BIT(7)
#define LIS3DHH_FIFO_SRC_OVRN BIT(6)
#define LIS3DHH_FIFO_SRC_FSS5 BIT(5)
#define LIS3DHH_FIFO_SRC_FSS4 BIT(4)
#define LIS3DHH_FIFO_SRC_FSS3 BIT(3)
#define LIS3DHH_FIFO_SRC_FSS2 BIT(2)
#define LIS3DHH_FIFO_SRC_FSS1 BIT(1)
#define LIS3DHH_FIFO_SRC_FSS0 BIT(0)

#define LIS3DHH_BUF_SZ 7

union lis3dhh_sample {
	uint8_t raw[LIS3DHH_BUF_SZ];
	uint8_t raw_temperature[2];
	struct {
		uint8_t status;
		uint16_t xyz[3];
		uint16_t ambient_temperature;
	} __packed;
};

struct lis3dhh_spi_cfg {
	struct spi_config spi_conf;
	const char *cs_gpios_label;
};

union lis3dhh_bus_cfg {
	const struct lis3dhh_spi_cfg *spi_cfg;
};

struct lis3dhh_config {
	const char *bus_name;
	int (*bus_init)(const struct device *dev);
	const union lis3dhh_bus_cfg bus_cfg;
};

struct lis3dhh_transfer_function {
	int (*read_data)(const struct device *dev, uint8_t reg_addr, uint8_t *value, uint8_t len);
	int (*write_data)(const struct device *dev, uint8_t reg_addr, uint8_t *value, uint8_t len);
	int (*read_reg)(const struct device *dev, uint8_t reg_addr, uint8_t *value);
	int (*write_reg)(const struct device *dev, uint8_t reg_addr, uint8_t value);
	int (*update_reg)(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value);
};

struct lis3dhh_data {
	const struct device *bus;
	const struct lis3dhh_transfer_function *hw_tf;
	union lis3dhh_sample sample;
	struct spi_cs_control cs_ctrl;
#if DT_INST_NODE_HAS_PROP(0, supply_gpios)
	const struct device *supply_gpios;
#endif
};

int lis3dhh_spi_init(const struct device *dev);

#endif /* __SENSOR_LIS3DHH__ */
