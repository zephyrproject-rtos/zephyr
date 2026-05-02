/*
 * Copyright (c) 2026 Filip Stojanovic <filipembedded@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADE7978_H_
#define ZEPHYR_DRIVERS_SENSOR_ADE7978_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor.h>

#define ADE7978_ADC_FULL_SCALE_OUTPUT  5320000.0
#define ADE7978_MAX_DIFF_CURRENT_INPUT 0.03125 /* 31.25 mV */
#define ADE7978_MAX_DIFF_VOLTAGE_INPUT 0.5     /* 0.5 V */

#define ADE7978_SHUNT_RESISTOR  (((double)CONFIG_ADE7978_SHUNT_RESISTOR) / 1e6)
#define ADE7978_VOLTAGE_DIVIDER (((double)CONFIG_ADE7978_VOLTAGE_DIVIDER))

#define ADE7978_IRATIO                                                                             \
	(ADE7978_ADC_FULL_SCALE_OUTPUT * ADE7978_SHUNT_RESISTOR / ADE7978_MAX_DIFF_CURRENT_INPUT)
#define ADE7978_VRATIO                                                                             \
	(ADE7978_ADC_FULL_SCALE_OUTPUT / ADE7978_VOLTAGE_DIVIDER / ADE7978_MAX_DIFF_VOLTAGE_INPUT)

/* ADE7978 register map */

/* System Control Registers */
#define ADE7978_REG_VERSION 0xE707 /* 8-bit  - Chip version */
#define ADE7978_REG_RUN     0xE228 /* 16-bit - Start/stop DSP */
#define ADE7978_REG_CONFIG  0xE618 /* 16-bit - Configuration */

/* Status and Interrupt Registers */
#define ADE7978_REG_STATUS0 0xE502 /* 32-bit - Interrupt status 0 */
#define ADE7978_REG_STATUS1 0xE503 /* 32-bit - Interrupt status 1 */
#define ADE7978_REG_MASK0   0xE50A /* 32-bit - Interrupt enable 0 */
#define ADE7978_REG_MASK1   0xE50B /* 32-bit - Interrupt enable 1 */

/* RMS Measurement Registers (24-bit data in 32-bit registers) */
#define ADE7978_REG_AIRMS 0x43C0 /* Phase A current RMS */
#define ADE7978_REG_BIRMS 0x43C3 /* Phase B current RMS */
#define ADE7978_REG_CIRMS 0x43C6 /* Phase C current RMS */
#define ADE7978_REG_NIRMS 0x43C9 /* Neutral current RMS */

#define ADE7978_REG_AVRMS 0x43C1 /* Phase A voltage RMS */
#define ADE7978_REG_BVRMS 0x43C4 /* Phase B voltage RMS */
#define ADE7978_REG_CVRMS 0x43C7 /* Phase C voltage RMS */
#define ADE7978_REG_NVRMS 0xE530 /* Neutral voltage RMS */

/* Instantaneous Power Registers (24-bit data) */
#define ADE7978_REG_AWATT 0xE518 /* Phase A active power */
#define ADE7978_REG_BWATT 0xE519 /* Phase B active power */
#define ADE7978_REG_CWATT 0xE51A /* Phase C active power */

#define ADE7978_REG_AVAR 0xE51B /* Phase A reactive power */
#define ADE7978_REG_BVAR 0xE51C /* Phase B reactive power */
#define ADE7978_REG_CVAR 0xE51D /* Phase C reactive power */

/* SPI Command bytes */
#define ADE7978_CMD_READ  0x01 /* SPI read command */
#define ADE7978_CMD_WRITE 0x00 /* SPI write command */
#define ADE7978_DUMMY     0xFF /* SPI dummy byte */

/* Helper macros for 24-bit sign extension */
#define ADE7978_24BIT_MASK   0x00FFFFFF
#define ADE7978_24BIT_SIGN   0x00800000
#define ADE7978_24BIT_EXTEND 0xFF000000

struct ade7978_config {
	struct spi_dt_spec spi;
};

struct ade7978_data {
	int32_t airms; /* Cached Phase A current RMS */
	int32_t avrms; /* Cached Phase A voltage RMS */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ADE7978_H_ */
