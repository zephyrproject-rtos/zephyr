/*
 * Copyright (c) 2020 ZedBlox Logitech Pvt Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HDC2080_HDC2080_H_
#define ZEPHYR_DRIVERS_SENSOR_HDC2080_HDC2080_H_

#include <zephyr/types.h>
#include <device.h>


#define HDC2080_CHIP_ID        0x07D0
#define HDC2080_MID            0x4954

/* Register map */
#define TEMP_LOW              0x00
#define TEMP_HIGH             0x01
#define HUMID_LOW             0x02
#define HUMID_HIGH            0x03
#define INTERRUPT_DRDY        0x04
#define TEMP_MAX              0x05
#define HUMID_MAX             0x06
#define INTERRUPT_CONFIG      0x07
#define TEMP_OFFSET_ADJUST    0x08
#define HUM_OFFSET_ADJUST     0x09
#define TEMP_THR_L            0x0A
#define TEMP_THR_H            0x0B
#define HUMID_THR_L           0x0C
#define HUMID_THR_H           0x0D
#define CONFIG                0x0E
#define MEASUREMENT_CONFIG    0x0F
#define MID_L                 0xFC
#define MID_H                 0xFD
#define DEVICE_ID_L           0xFE
#define DEVICE_ID_H           0xFF

#define AMM_MODE_ONE_HZ       0x5
#define AMM_MODE_OFFSET       0x1

#endif /* ZEPHYR_DRIVERS_SENSOR_HDC2080_HDC2080_H_ */
