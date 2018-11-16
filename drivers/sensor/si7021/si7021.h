/*
 * Copyright (c) 2018 Thomas Li Fredriksen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SI7021_SI7021_H_
#define ZEPHYR_DRIVERS_SENSOR_SI7021_SI7021_H_

#define SI7021_DEFAULT_ADDRESS	0x40

#define SI7021_MEASRH_HOLD_CMD          0xE5    /* Measure Relative Humidity, Hold Master Mode*/
#define SI7021_MEASRH_NOHOLD_CMD        0xF5    /* Measure Relative Humidity, No Hold Master Mode*/
#define SI7021_MEASTEMP_HOLD_CMD        0xE3    /* Measure Temperature, Hold Master Mode*/
#define SI7021_MEASTEMP_NOHOLD_CMD      0xF3    /* Measure Temperature, No Hold Master Mode*/
#define SI7021_READPREVTEMP_CMD         0xE0    /* Read Temperature Value from Previous RH Measurement*/
#define SI7021_RESET_CMD                0xFE
#define SI7021_WRITERHT_REG_CMD         0xE6    /* Write RH/T User Register 1*/
#define SI7021_READRHT_REG_CMD          0xE7    /* Read RH/T User Register 1*/
#define SI7021_WRITEHEATER_REG_CMD      0x51    /* Write Heater Control Register*/
#define SI7021_READHEATER_REG_CMD       0x11    /* Read Heater Control Register*/
#define SI7021_ID1_CMD                  0xFA0F  /* Read Electronic ID 1st Byte*/
#define SI7021_ID2_CMD                  0xFCC9  /* Read Electronic ID 2nd Byte*/
#define SI7021_FIRMVERS_CMD             0x84B8  /* Read Firmware Revision*/

#define SI7021_REV_1					0xff
#define SI7021_REV_2					0x20

struct si7021_data {
    
    int dummy;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SI7021_SI7021_H_ */