/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA7xx_INA7xx_H_
#define ZEPHYR_DRIVERS_SENSOR_INA7xx_INA7xx_H_

/* Config register shifts and masks */
#define INA7xx_REG_CONFIG    0x00
#define INA7xx_RST_MASK      0x1
#define INA7xx_RST_SHIFT     15
#define INA7xx_RSTACC_MASK   0x1
#define INA7xx_RSTACC_SHIFT  14
#define INA7xx_CONVDLY_MASK  0xff
#define INA7xx_CONVDLY_SHIFT 6

/* Mode fields */
/* Shutdown */
#define INA7xx_MODE_OFF     0x0
/* Triggered mode for temp, current and bus voltage, single shot */
#define INA7xx_MODE_TRIGGER 0x7
/* Continuous temperature, current, and bus voltage */
#define INA7xx_MODE_CONTI   0xf

/* ADC Config register fields, shifts and masks */
#define INA7xx_REG_ADC_CONFIG 0x01
#define INA7xx_MODE_MASK      0xf
#define INA7xx_MODE_SHIFT     12
#define INA7xx_REG_ADC_CONFIG 0x01
#define INA7xx_MODE           GENMASK(15, 12)
#define INA7xx_VBUSCT         GENMASK(11, 9)
#define INA7xx_VSENCT         GENMASK(8, 6)
#define INA7xx_TCT            GENMASK(5, 3)
#define INA7xx_AVG            GENMASK(2, 0)

#define INA7xx_REG_BUS_VOLTAGE    0x05
#define INA7xx_REG_CURRENT        0x07
#define INA7xx_REG_DIE_TEMP       0x06
#define INA7xx_REG_DIE_TEMP_SHIFT 4
#define INA7xx_REG_POWER          0x08
#define INA7xx_REG_ENERGY         0x09
#define INA7xx_REG_CHARGE         0x03
#define INA7xx_REG_DIAG_ALRT      0x0b
#define INA7xx_REG_COL            0x0c
#define INA7xx_REG_CUL            0x0d
#define INA7xx_REG_BOVL           0x0e
#define INA7xx_REG_BUVL           0x0f
#define INA7xx_REG_TEMP_LIMIT     0x10
#define INA7xx_REG_PWR_LIMIT      0x11
#define INA7xx_REG_ID             0x3e

#define VENDOR_ID 0x5449

/* Others */
#define INA7xx_SIGN_BIT(x)       ((x) >> 15) & 0x1
#define INA7xx_BUS_VOLTAGE_MUL   0.003125
#define INA7xx_TEMP_SCALE        0.125
#define INA7xx_WAIT_STARTUP_USEC 40

#define INA700_CURRENT_MUL 0.00048
#define INA700_POWER_MUL   0.000096
#define INA745_CURRENT_MUL 0.0012
#define INA745_POWER_MUL   0.000240
#define INA780_CURRENT_MUL 0.0024
#define INA780_POWER_MUL   0.000480

#define INA7xx_HAS_VOLTAGE_MASK  BIT(0)
#define INA7xx_HAS_DIE_TEMP_MASK BIT(1)
#define INA7xx_HAS_POWER_MASK    BIT(2)
#define INA7xx_HAS_CURRENT_MASK  BIT(3)

#define DEVICE_TYPE_INA700 1
#define DEVICE_TYPE_INA745 2
#define DEVICE_TYPE_INA780 3

#endif /* ZEPHYR_DRIVERS_SENSOR_INA7xx_INA7xx_H_ */
