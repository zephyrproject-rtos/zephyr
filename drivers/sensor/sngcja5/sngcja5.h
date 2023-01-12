/*
 * Copyright (c) 2023 Panasonic Industrial Devices Europe GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SNGCJA5_SNGCJA5_H_
#define ZEPHYR_DRIVERS_SENSOR_SNGCJA5_SNGCJA5_H_

#include <zephyr/drivers/i2c.h>

/*Register for PM1.0*/
#define SNGCJA5_PM10_LL	 0x00
#define SNGCJA5_PM10_LH	 0x01
#define SNGCJA5_PM10_HL	 0x02
#define SNGCJA5_PM10_HH	 0x03
/*Register for PM2.5*/
#define SNGCJA5_PM25_LL	 0x04
#define SNGCJA5_PM25_LH	 0x05
#define SNGCJA5_PM25_HL	 0x06
#define SNGCJA5_PM25_HH	 0x07
/*Register for PM10*/
#define SNGCJA5_PM100_LL 0x08
#define SNGCJA5_PM100_LH 0x09
#define SNGCJA5_PM100_HL 0x0a
#define SNGCJA5_PM100_HH 0x0b

/*Register 1 for particle count (0.3-0.5µm)*/
#define SNGCJA5_05_L  0x0c
#define SNGCJA5_05_H  0x0d
/*Register 2 for particle count (0.5-1.0µm)*/
#define SNGCJA5_10_L  0x0e
#define SNGCJA5_10_H  0x0f
/*Register 3 for particle count (1.0-2.5µm)*/
#define SNGCJA5_25_L  0x10
#define SNGCJA5_25_H  0x11
/*Register 4 for particle count (2.5-5.0µm)*/
#define SNGCJA5_50_L  0x14
#define SNGCJA5_50_H  0x15
/*Register 5 for particle count (5.0-7.5µm)*/
#define SNGCJA5_75_L  0x16
#define SNGCJA5_75_H  0x17
/*Register 6 for particle count (7.5-10.0µm)*/
#define SNGCJA5_100_L 0x18
#define SNGCJA5_100_H 0x19

/*Register for sensor status information*/
#define SNGCJA5_STATUS 0x26

/*Status flag masks*/
#define SNGCJA5_STATUS_SENSOR_STATUS_MASK 0xc0

#define SNGCJA5_STATUS_PD_STATUS_MASK	   0x30
#define SNGCJA5_STATUS_PD_STATUS_NORMAL	   0x00
#define SNGCJA5_STATUS_PD_STATUS_WITHIN_80 0x10
#define SNGCJA5_STATUS_PD_STATUS_BELOW_90  0x20
#define SNGCJA5_STATUS_PD_STATUS_BELOW_80  0x30

#define SNGCJA5_STATUS_LD_STATUS_MASK	   0x0c
#define SNGCJA5_STATUS_LD_STATUS_NORMAL	   0x00
#define SNGCJA5_STATUS_LD_STATUS_WITHIN_70 0x04
#define SNGCJA5_STATUS_LD_STATUS_BELOW_90  0x08
#define SNGCJA5_STATUS_LD_STATUS_BELOW_70  0x0c

#define SNGCJA5_STATUS_FAN_STATUS_MASK		  0x03
#define SNGCJA5_STATUS_FAN_STATUS_NORMAL	  0x00
#define SNGCJA5_STATUS_FAN_STATUS_1000RPM_OR_MORE 0x01
#define SNGCJA5_STATUS_FAN_STATUS_IN_CALIBRATION  0x02
#define SNGCJA5_STATUS_FAN_STATUS_ABNORMAL	  0x03

#define SNGCJA5_SCALE_FACTOR 1000

struct sngcja5_config {
	const struct i2c_dt_spec i2c;
};

struct sngcja5_data {
	uint32_t pm1_0;
	uint32_t pm2_5;
	uint32_t pm10_0;
	uint16_t pc0_5;
	uint16_t pc1_0;
	uint16_t pc2_5;
	uint16_t pc5_0;
	uint16_t pc7_5;
	uint16_t pc10_0;
};

#endif
