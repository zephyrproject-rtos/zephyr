/*
 * Copyright (c) 2025 Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of Atmel M90E26 energy metering IC
 * @ingroup m90e26_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_M90E26_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_M90E26_H_

/**
 * @defgroup m90e26_interface M90E26 Interface
 * @brief Documentation for Atmel M90E26 energy metering IC interface
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Bit masks for SysStatus register fields.
 */

#define M90E26_SYSSTATUS_CALERR_BIT_MASK   (BIT(14) | BIT(15))
#define M90E26_SYSSTATUS_ADJERR_BIT_MASK   (BIT(12) | BIT(13))
#define M90E26_SYSSTATUS_LNCHANGE_BIT_MASK BIT(7)
#define M90E26_SYSSTATUS_REVQCHQ_BIT_MASK  BIT(6)
#define M90E26_SYSSTATUS_REVPCHG_BIT_MASK  BIT(5)
#define M90E26_SYSSTATUS_SAGWARN_BIT_MASK  BIT(1)

/**
 * @brief Type definition for M90E26 register addresses.
 */
typedef uint8_t m90e26_register_t;

/**
 * @brief Type definition for M90E26 register addresses.
 */
typedef union m90e26_data_value {
	int16_t int16;
	uint16_t uint16;
} m90e26_data_value_t;

/**
 * @brief Custom sensor channels for Atmel M90E26.
 */
enum m90e26_sensor_channel {
	M90E26_SENSOR_CHANNEL_ENERGY = SENSOR_CHAN_PRIV_START + 1,
	M90E26_SENSOR_CHANNEL_POWER,
	M90E26_SENSOR_CHANNEL_VOLTAGE,
	M90E26_SENSOR_CHANNEL_CURRENT,
	M90E26_SENSOR_CHANNEL_FREQUENCY,
	M90E26_SENSOR_CHANNEL_PHASE_ANGLE,
	M90E26_SENSOR_CHANNEL_POWER_FACTOR,
};

/**
 * @brief Custom sensor trigger types for Atmel M90E26.
 */
enum m90e26_sensor_trigger_type {
	M90E26_SENSOR_TRIG_TYPE_CF1 = SENSOR_TRIG_PRIV_START + 1,
	M90E26_SENSOR_TRIG_TYPE_CF2,
	M90E26_SENSOR_TRIG_TYPE_IRQ,
	M90E26_SENSOR_TRIG_TYPE_WRN_OUT,
};

/**
 * @brief m90e26_sensor_data Structure.
 *
 * This structure holds all the measurement data read from the M90E26 energy metering IC
 * converted to sensor_value format.
 */
struct m90e26_sensor_data {
	struct m90e26_energy_sensor_data {
		struct sensor_value APenergy;
		struct sensor_value ANenergy;
		struct sensor_value ATenergy;
		struct sensor_value RPenergy;
		struct sensor_value RNenergy;
		struct sensor_value RTenergy;
	} energy_sensor_values;

	struct m90e26_power_sensor_data {
		struct sensor_value Pmean;  /* [W] */
		struct sensor_value Qmean;  /* [VAR] */
		struct sensor_value Smean;  /* [VA] */
		struct sensor_value Pmean2; /* [W] */
		struct sensor_value Qmean2; /* [VAR] */
		struct sensor_value Smean2; /* [VA] */
	} power_sensor_values;

	struct sensor_value Urms; /* [V] */

	struct m90e26_current_sensor_data {
		struct sensor_value Irms;  /* [A] */
		struct sensor_value Irms2; /* [A] */
	} current_sensor_values;

	struct sensor_value Freq; /* [Hz] */

	struct m90e26_phase_angle_sensor_data {
		struct sensor_value Pangle;  /* [degrees] */
		struct sensor_value Pangle2; /* [degrees] */
	} phase_angle_sensor_values;

	struct m90e26_power_factor_sensor_data {
		struct sensor_value PowerF;  /* [unitless] */
		struct sensor_value PowerF2; /* [unitless] */
	} power_factor_sensor_values;
};

/**
 * @brief m90e26_config_registers Structure.
 *
 * This structure holds the configuration registers of the M90E26 device.
 */
struct m90e26_config_registers {
	/* Special Registers */

	m90e26_data_value_t FuncEn;
	m90e26_data_value_t SagTh;
	m90e26_data_value_t SmallPMod;

	/* Metering Calibration and Condiguration Registers */

	m90e26_data_value_t PLconstH;
	m90e26_data_value_t PLconstL;
	m90e26_data_value_t Lgain;
	m90e26_data_value_t Lphi;
	m90e26_data_value_t Ngain;
	m90e26_data_value_t Nphi;
	m90e26_data_value_t PStartTh;
	m90e26_data_value_t PNolTh;
	m90e26_data_value_t QStartTh;
	m90e26_data_value_t QNolTh;
	m90e26_data_value_t MMode;

	/* Measurement Calibration Registers */

	m90e26_data_value_t Ugain;
	m90e26_data_value_t IgainL;
	m90e26_data_value_t IgainN;
	m90e26_data_value_t Uoffset;
	m90e26_data_value_t IoffsetL;
	m90e26_data_value_t IoffsetN;
	m90e26_data_value_t PoffsetL;
	m90e26_data_value_t QoffsetL;
	m90e26_data_value_t PoffsetN;
	m90e26_data_value_t QoffsetN;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_M90E26_H_ */
