/*
 * Copyright (c) 2022 HAW Hamburg FTZ-DIWIP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _MAX31865_H
#define _MAX31865_H

#define DT_DRV_COMPAT maxim_max31865

#include <math.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/max31865.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/byteorder.h>
LOG_MODULE_REGISTER(MAX31865, CONFIG_SENSOR_LOG_LEVEL);

#define MAX31865_FAULT_HIGH_THRESHOLD BIT(7)
#define MAX31865_FAULT_LOW_THRESHOLD  BIT(6)
#define MAX31865_FAULT_REFIN	      BIT(5)
#define MAX31865_FAULT_REFIN_FORCE    BIT(4)
#define MAX31865_FAULT_RTDIN_FORCE    BIT(3)
#define MAX31865_FAULT_VOLTAGE	      BIT(2)

#define MAX31865_FAULT_DETECTION_NONE	  (0x00 << 2)
#define MAX31865_FAULT_DETECTION_AUTO	  (0x01 << 2)
#define MAX31865_FAULT_DETECTION_MANUAL_1 (0x02 << 2)
#define MAX31865_FAULT_DETECTION_MANUAL_2 (0x03 << 2)

/* Read Register Address */
#define REG_CONFIG	       0x00
#define REG_RTD_MSB	       0x01
#define REG_RTD_LSB	       0x02
#define REG_HIGH_FAULT_THR_MSB 0x03
#define REG_HIGH_FAULT_THR_LSB 0x04
#define REG_LOW_FAULT_THR_MSB  0x05
#define REG_LOW_FAULT_THR_LSB  0x06
#define REG_FAULT_STATUS       0x07
#define WR(reg)		       ((reg) | 0x80)

/**
 * RTD data, RTD current, and measurement reference
 * voltage. The ITS-90 standard is used; other RTDs
 * may have coefficients defined by the DIN 43760 or
 * the U.S. Industrial (American) standard.
 */

#define RTD_A_ITS90	   3.9080e-3
#define RTD_A_USINDUSTRIAL 3.9692e-3
#define RTD_A_DIN43760	   3.9848e-3
#define RTD_B_ITS90	   -5.870e-7
#define RTD_B_USINDUSTRIAL -5.8495e-7
#define RTD_B_DIN43760	   -5.8019e-7

/**
 * RTD coefficient C is required only for temperatures
 * below 0 deg. C.  The selected RTD coefficient set
 * is specified below.
 */
#define RTD_A (RTD_A_ITS90)
#define RTD_B (RTD_B_ITS90)

/*
 * For under zero, taken from
 * https://www.analog.com/media/en/technical-documentation/application-notes/AN709_0.pdf
 */
static const float A[6] = {-242.02, 2.2228, 2.5859e-3, 4.8260e-6, 2.8183e-8, 1.5243e-10};

struct max31865_data {
	double temperature;
	uint8_t config_control_bits;
};

/**
 * Configuration struct to the MAX31865.
 */
struct max31865_config {
	const struct spi_dt_spec spi;
	uint16_t resistance_at_zero;
	uint16_t resistance_reference;
	bool conversion_mode;
	bool one_shot;
	bool three_wire;
	uint8_t fault_cycle;
	bool filter_50hz;
	uint16_t low_threshold;
	uint16_t high_threshold;
};

/* Bit manipulation macros */
#define TESTBIT(data, pos) ((0u == (data & BIT(pos))) ? 0u : 1u)

#endif
