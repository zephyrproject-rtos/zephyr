/**
 * Copyright (c) 2025 Orgatex GmbH
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file bq35100.h
 * @brief Driver interface for the BQ35100 gas gauge
 *
 * Technical Reference:
 * - Texas Instruments BQ35100: https://www.ti.com/product/de-de/BQ35100
 */

#ifndef ZEPHYR_DRIVERS_FUEL_GAUGE_BQ35100_H_
#define ZEPHYR_DRIVERS_FUEL_GAUGE_BQ35100_H_

#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

/* Data flash addresses */
#define BQ35100_FLASH_SERIES_CELL_COUNT         0x4206
#define BQ35100_FLASH_OPERATION_CONFIG_A        0x41B1
#define BQ35100_FLASH_ALERT_CONFIG              0x41B2
#define BQ35100_FLASH_VIN_GAIN                  0x4010
#define BQ35100_FLASH_BAT_LOW_VOLTAGE_THRESHOLD 0x41DB
#define BQ35100_FLASH_FULL_ACCESS_CODES         0x41D0
#define BQ35100_FLASH_CMD_SET_NEW_CAPACITY      0x41FE
#define BQ35100_FLASH_CC_OFFSET                 0x4008
#define BQ35100_FLASH_BOARD_OFFSET              0x400C
#define BQ35100_FLASH_CC_GAIN                   0x4000
#define BQ35100_FLASH_CC_DELTA                  0x4004

#define BQ35100_DEVICE_TYPE 0x100

/* Register addresses */
#define BQ35100_REG_CONTROL_STATUS       0x00
#define BQ35100_REG_ACCUMULATED_CAPACITY 0x02
#define BQ35100_REG_TEMPERATURE          0x06
#define BQ35100_REG_VOLTAGE              0x08
#define BQ35100_REG_BATTERY_STATUS       0x0A
#define BQ35100_REG_BATTERY_ALERT        0x0B
#define BQ35100_REG_CURRENT              0x0C
#define BQ35100_REG_SCALED_R             0x16
#define BQ35100_REG_MEASURED_Z           0x22
#define BQ35100_REG_INTERNAL_TEMPERATURE 0x28
#define BQ35100_REG_STATE_OF_HEALTH      0x2E
#define BQ35100_REG_DESIGN_CAPACITY      0x3C
#define BQ35100_REG_MAC                  0x3E
#define BQ35100_REG_MAC_DATA             0x40
#define BQ35100_REG_MAC_DATA_SUM         0x60
#define BQ35100_REG_MAC_DATA_LEN         0x61
#define BQ35100_REG_CAL_COUNT            0x79
#define BQ35100_REG_CAL_CURRENT          0x7A
#define BQ35100_REG_CAL_VOLTAGE          0x7C
#define BQ35100_REG_CAL_TEMPERATURE      0x7E

/* MAC commands */
#define BQ35100_MAC_CMD_CONTROL_STATUS     0x0000
#define BQ35100_MAC_CMD_DEVICETYPE         0x0001
#define BQ35100_MAC_CMD_FIRMWAREVERSION    0x0002
#define BQ35100_MAC_CMD_HARDWAREVERSION    0x0003
#define BQ35100_MAC_CMD_STATIC_CHEM_CHKSUM 0x0005
#define BQ35100_MAC_CMD_CHEMID             0x0006
#define BQ35100_MAC_CMD_PREV_MACWRITE      0x0007
#define BQ35100_MAC_CMD_BOARD_OFFSET       0x0009
#define BQ35100_MAC_CMD_CC_OFFSET          0x000A
#define BQ35100_MAC_CMD_CC_OFFSET_SAVE     0x000B
#define BQ35100_MAC_CMD_GAUGE_START        0x0011
#define BQ35100_MAC_CMD_GAUGE_STOP         0x0012
#define BQ35100_MAC_CMD_SEALED             0x0020
#define BQ35100_MAC_CMD_CAL_ENABLE         0x002D
#define BQ35100_MAC_CMD_LT_ENABLE          0x002E
#define BQ35100_MAC_CMD_RESET              0x0041
#define BQ35100_MAC_CMD_EXIT_CAL           0x0080
#define BQ35100_MAC_CMD_ENTER_CAL          0x0081
#define BQ35100_MAC_CMD_NEW_BATTERY        0xA613

/* Bit masks*/
#define BQ3500_CCA_BIT_MASK         BIT(10)
#define BQ3500_BCA_BIT_MASK         BIT(11)
#define BQ3500_INITCOMP_BIT_MASK    BIT(7)
#define BQ35100_GA_BIT_MASK         BIT(0)
#define BQ35100_G_DONE_BIT_MASK     BIT(6)
#define BQ35100_SEC_MODE_BIT_MASK   (BIT(13) & BIT(14))
#define BQ35100_FLASH_FAIL_BIT_MASK BIT(15)

/** The default seal codes (step 1 in the higher word, step 2 the lower word), NOT byte reversed */
#define BQ35100_DEFAULT_SEAL_CODES 0x04143672

struct bq35100_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec supply_gpio;
};

typedef enum {
	SECURITY_UNKNOWN = 0x00,
	SECURITY_FULL_ACCESS = 0x01,
	SECURITY_UNSEALED = 0x02,
	SECURITY_SEALED = 0x03
} bq35100_security_t;

bq35100_security_t get_intern_sec_mode(void);

int bq35100_send_cntl(const struct device *dev, uint16_t cntl_address);

int bq35100_get_status(const struct device *dev, uint16_t *status);

int bq35100_get_data(const struct device *dev, uint8_t address, uint8_t *data, size_t len);

int bq35100_write_extended_data(const struct device *dev, const uint16_t address,
				const uint8_t *data, size_t len);

int bq35100_read_extended_data(const struct device *dev, const uint16_t address, uint8_t *data,
			       size_t len);

#endif /* ZEPHYR_DRIVERS_FUEL_GAUGE_BQ35100_H_ */
