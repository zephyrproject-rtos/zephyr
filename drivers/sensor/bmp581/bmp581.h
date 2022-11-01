/*
 * Copyright (c) 2022 Badgerd Technologies B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver is developed to be used with Zephyr. And it only supports i2c interface.
 *
 * Author: Talha Can Havadar <havadartalha@gmail.com>
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_H_
#define ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/drivers/sensor/bmp581_user.h>

#define DT_DRV_COMPAT bosch_bmp581

/* UTILITY MACROS */
#define BMP5_SET_LOW_BYTE  0x00FFu
#define BMP5_SET_HIGH_BYTE 0xFF00u

/* BIT SLICE GET AND SET FUNCTIONS */
#define BMP5_GET_BITSLICE(regvar, bitname) ((regvar & bitname##_MSK) >> bitname##_POS)

#define BMP5_SET_BITSLICE(regvar, bitname, val)                                                    \
	((regvar & ~bitname##_MSK) | ((val << bitname##_POS) & bitname##_MSK))

#define BMP5_GET_LSB(var) (uint8_t)(var & BMP5_SET_LOW_BYTE)
#define BMP5_GET_MSB(var) (uint8_t)((var & BMP5_SET_HIGH_BYTE) >> 8)

#define BMP5_SET_BIT_VAL_0(reg_data, bitname) (reg_data & ~(bitname##_MSK))

#define BMP5_SET_BITS_POS_0(reg_data, bitname, data)                                               \
	((reg_data & ~(bitname##_MSK)) | (data & bitname##_MSK))

#define BMP5_GET_BITS_POS_0(reg_data, bitname) (reg_data & (bitname##_MSK))

#define BMP5_OK      0
#define BMP5_ENABLE  1u
#define BMP5_DISABLE 0u

/* BMP5 Registers */
#define BMP5_REG_CHIP_ID         0x01
#define BMP5_REG_REV_ID          0x02
#define BMP5_REG_CHIP_STATUS     0x11
#define BMP5_REG_DRIVE_CONFIG    0x13
#define BMP5_REG_INT_CONFIG      0x14
#define BMP5_REG_INT_SOURCE      0x15
#define BMP5_REG_FIFO_CONFIG     0x16
#define BMP5_REG_FIFO_COUNT      0x17
#define BMP5_REG_FIFO_SEL        0x18
#define BMP5_REG_TEMP_DATA_XLSB  0x1D
#define BMP5_REG_TEMP_DATA_LSB   0x1E
#define BMP5_REG_TEMP_DATA_MSB   0x1F
#define BMP5_REG_PRESS_DATA_XLSB 0x20
#define BMP5_REG_PRESS_DATA_LSB  0x21
#define BMP5_REG_PRESS_DATA_MSB  0x22
#define BMP5_REG_INT_STATUS      0x27
#define BMP5_REG_STATUS          0x28
#define BMP5_REG_FIFO_DATA       0x29
#define BMP5_REG_NVM_ADDR        0x2B
#define BMP5_REG_NVM_DATA_LSB    0x2C
#define BMP5_REG_NVM_DATA_MSB    0x2D
#define BMP5_REG_DSP_CONFIG      0x30
#define BMP5_REG_DSP_IIR         0x31
#define BMP5_REG_OOR_THR_P_LSB   0x32
#define BMP5_REG_OOR_THR_P_MSB   0x33
#define BMP5_REG_OOR_RANGE       0x34
#define BMP5_REG_OOR_CONFIG      0x35
#define BMP5_REG_OSR_CONFIG      0x36
#define BMP5_REG_ODR_CONFIG      0x37
#define BMP5_REG_OSR_EFF         0x38
#define BMP5_REG_CMD             0x7E
/* endof BMP5 Registers */

/* Chip id of BMP5 */
#define BMP5_CHIP_ID_PRIM 0x50
#define BMP5_CHIP_ID_SEC  0x51

/* I2C addresses */
#define BMP5_I2C_ADDR_PRIM 0x46
#define BMP5_I2C_ADDR_SEC  0x47

/* NVM addresses */
#define BMP5_NVM_START_ADDR 0x20
#define BMP5_NVM_END_ADDR   0x22

/* Interface settings */
#define BMP5_SPI_RD_MASK 0x80

/* Delay definition */
#define BMP5_DELAY_US_SOFT_RESET      2000
#define BMP5_DELAY_US_STANDBY         2500
#define BMP5_DELAY_US_NVM_READY_READ  800
#define BMP5_DELAY_US_NVM_READY_WRITE 10000

/* Soft reset command */
#define BMP5_SOFT_RESET_CMD 0xB6

/*! NVM command */
#define BMP5_NVM_FIRST_CMND        0x5D
#define BMP5_NVM_READ_ENABLE_CMND  0xA5
#define BMP5_NVM_WRITE_ENABLE_CMND 0xA0

/* Deepstandby enable/disable */
#define BMP5_DEEP_ENABLED  0
#define BMP5_DEEP_DISABLED 1

/*! Fifo frame configuration */
#define BMP5_FIFO_EMPTY                  0X7F
#define BMP5_FIFO_MAX_THRESHOLD_P_T_MODE 0x0F
#define BMP5_FIFO_MAX_THRESHOLD_P_MODE   0x1F

/* Macro is used to bypass both iir_t and iir_p together */
#define BMP5_IIR_BYPASS 0xC0

/* Pressure Out-of-range count limit */
#define BMP5_OOR_COUNT_LIMIT_1  0x00
#define BMP5_OOR_COUNT_LIMIT_3  0x01
#define BMP5_OOR_COUNT_LIMIT_7  0x02
#define BMP5_OOR_COUNT_LIMIT_15 0x03

/* Interrupt configurations */
#define BMP5_INT_MODE_PULSED  0
#define BMP5_INT_MODE_LATCHED 1

#define BMP5_INT_POL_ACTIVE_LOW  0
#define BMP5_INT_POL_ACTIVE_HIGH 1

#define BMP5_INT_OD_PUSHPULL  0
#define BMP5_INT_OD_OPENDRAIN 1

/* NVM and Interrupt status asserted macros */
#define BMP5_INT_ASSERTED_DRDY                   0x01
#define BMP5_INT_ASSERTED_FIFO_FULL              0x02
#define BMP5_INT_ASSERTED_FIFO_THRES             0x04
#define BMP5_INT_ASSERTED_PRESSURE_OOR           0x08
#define BMP5_INT_ASSERTED_POR_SOFTRESET_COMPLETE 0x10
#define BMP5_INT_NVM_RDY                         0x02
#define BMP5_INT_NVM_ERR                         0x04
#define BMP5_INT_NVM_CMD_ERR                     0x08

/* Interrupt configurations */
#define BMP5_INT_MODE_MSK 0x01

#define BMP5_INT_POL_MSK 0x02
#define BMP5_INT_POL_POS 1

#define BMP5_INT_OD_MSK 0x04
#define BMP5_INT_OD_POS 2

#define BMP5_INT_EN_MSK 0x08
#define BMP5_INT_EN_POS 3

#define BMP5_INT_DRDY_EN_MSK 0x01

#define BMP5_INT_FIFO_FULL_EN_MSK 0x02
#define BMP5_INT_FIFO_FULL_EN_POS 1

#define BMP5_INT_FIFO_THRES_EN_MSK 0x04
#define BMP5_INT_FIFO_THRES_EN_POS 2

#define BMP5_INT_OOR_PRESS_EN_MSK 0x08
#define BMP5_INT_OOR_PRESS_EN_POS 3

/* ODR configuration */
#define BMP5_ODR_MSK 0x7C
#define BMP5_ODR_POS 2

/* OSR configurations */
#define BMP5_TEMP_OS_MSK 0x07

#define BMP5_PRESS_OS_MSK 0x38
#define BMP5_PRESS_OS_POS 3

/* Pressure enable */
#define BMP5_PRESS_EN_MSK 0x40
#define BMP5_PRESS_EN_POS 6

/* IIR configurations */
#define BMP5_SET_IIR_TEMP_MSK 0x07

#define BMP5_SET_IIR_PRESS_MSK 0x38
#define BMP5_SET_IIR_PRESS_POS 3

#define BMP5_OOR_SEL_IIR_PRESS_MSK 0x80
#define BMP5_OOR_SEL_IIR_PRESS_POS 7

#define BMP5_SHDW_SET_IIR_TEMP_MSK 0x08
#define BMP5_SHDW_SET_IIR_TEMP_POS 3

#define BMP5_SHDW_SET_IIR_PRESS_MSK 0x20
#define BMP5_SHDW_SET_IIR_PRESS_POS 5

#define BMP5_SET_FIFO_IIR_TEMP_MSK 0x10
#define BMP5_SET_FIFO_IIR_TEMP_POS 4

#define BMP5_SET_FIFO_IIR_PRESS_MSK 0x40
#define BMP5_SET_FIFO_IIR_PRESS_POS 6

#define BMP5_IIR_FLUSH_FORCED_EN_MSK 0x04
#define BMP5_IIR_FLUSH_FORCED_EN_POS 2

/* Effective OSR configurations and ODR valid status */
#define BMP5_OSR_TEMP_EFF_MSK 0x07

#define BMP5_OSR_PRESS_EFF_MSK 0x38
#define BMP5_OSR_PRESS_EFF_POS 3

#define BMP5_ODR_IS_VALID_MSK 0x80
#define BMP5_ODR_IS_VALID_POS 7

/* Powermode */
#define BMP5_POWERMODE_MSK 0x03

#define BMP5_DEEP_DISABLE_MSK 0x80
#define BMP5_DEEP_DISABLE_POS 7

/* Fifo configurations */
#define BMP5_FIFO_THRESHOLD_MSK 0x1F

#define BMP5_FIFO_MODE_MSK 0x20
#define BMP5_FIFO_MODE_POS 5

#define BMP5_FIFO_DEC_SEL_MSK 0x1C
#define BMP5_FIFO_DEC_SEL_POS 2

#define BMP5_FIFO_COUNT_MSK 0x3F

#define BMP5_FIFO_FRAME_SEL_MSK 0x03

/* Out-of-range configuration */
#define BMP5_OOR_THR_P_LSB_MSK 0x0000FF

#define BMP5_OOR_THR_P_MSB_MSK 0x00FF00

#define BMP5_OOR_THR_P_XMSB_MSK 0x010000
#define BMP5_OOR_THR_P_XMSB_POS 16

/* Macro to mask xmsb value of oor threshold from register(0x35) value */
#define BMP5_OOR_THR_P_XMSB_REG_MSK 0x01

#define BMP5_OOR_COUNT_LIMIT_MSK 0xC0
#define BMP5_OOR_COUNT_LIMIT_POS 6

/* NVM configuration */
#define BMP5_NVM_ADDR_MSK 0x3F

#define BMP5_NVM_PROG_EN_MSK 0x40
#define BMP5_NVM_PROG_EN_POS 6

#define BMP5_NVM_DATA_LSB_MSK 0x00FF

#define BMP5_NVM_DATA_MSB_MSK 0xFF00

/*!
 * @brief OSR, ODR and pressure configuration structure
 */
struct bmp581_osr_odr_press_config {
	/*! Temperature oversampling
	 * Assignable macros :
	 * - BMP5_OVERSAMPLING_1X
	 * - BMP5_OVERSAMPLING_2X
	 * - BMP5_OVERSAMPLING_4X
	 * - BMP5_OVERSAMPLING_8X
	 * - BMP5_OVERSAMPLING_16X
	 * - BMP5_OVERSAMPLING_32X
	 * - BMP5_OVERSAMPLING_64X
	 * - BMP5_OVERSAMPLING_128X
	 */
	uint8_t osr_t;

	/*! Pressure oversampling
	 * Assignable macros :
	 * - BMP5_OVERSAMPLING_1X
	 * - BMP5_OVERSAMPLING_2X
	 * - BMP5_OVERSAMPLING_4X
	 * - BMP5_OVERSAMPLING_8X
	 * - BMP5_OVERSAMPLING_16X
	 * - BMP5_OVERSAMPLING_32X
	 * - BMP5_OVERSAMPLING_64X
	 * - BMP5_OVERSAMPLING_128X
	 */
	uint8_t osr_p;

	/*! Enable pressure
	 * BMP5_ENABLE  = Enables pressure data
	 * BMP5_DISABLE = Disables pressure data
	 */
	uint8_t press_en;

	/*! Output Data Rate */
	uint8_t odr;
};

struct bmp581_sample {
	struct sensor_value pressure;
	struct sensor_value temperature;
};

struct bmp581_data {
	uint8_t chip_id;
	struct bmp581_sample last_sample;
	struct bmp581_osr_odr_press_config osr_odr_press_config;
};

struct bmp581_config {
	struct i2c_dt_spec i2c;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_H_ */
