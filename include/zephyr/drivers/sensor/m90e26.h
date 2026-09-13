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
 * @ingroup sensor_interface_ext_atmel
 * @brief Documentation for Atmel M90E26 energy metering IC interface
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/* Bit masks for SysStatus register fields. */

/** @brief Metering calibration checksum (CS1) error. */
#define M90E26_SYSSTATUS_CALERR_BIT_MASK   (BIT(14) | BIT(15))
/** @brief Measurement calibration checksum (CS2) error. */
#define M90E26_SYSSTATUS_ADJERR_BIT_MASK   (BIT(12) | BIT(13))
/** @brief Line frequency changed between 50 Hz and 60 Hz. */
#define M90E26_SYSSTATUS_LNCHANGE_BIT_MASK BIT(7)
/** @brief Reactive energy accumulation direction (sign) changed. */
#define M90E26_SYSSTATUS_REVQCHQ_BIT_MASK  BIT(6)
/** @brief Active energy accumulation direction (sign) changed. */
#define M90E26_SYSSTATUS_REVPCHG_BIT_MASK  BIT(5)
/** @brief RMS voltage sag warning. */
#define M90E26_SYSSTATUS_SAGWARN_BIT_MASK  BIT(1)

/**
 * @brief Type definition for M90E26 register addresses.
 */
typedef uint8_t m90e26_register_t;

/**
 * @brief Raw 16-bit register value, viewable as signed or unsigned.
 */
typedef union m90e26_data_value {
	int16_t int16;   /**< Value interpreted as a signed 16-bit integer. */
	uint16_t uint16; /**< Value interpreted as an unsigned 16-bit integer. */
} m90e26_data_value_t;

/**
 * @brief Custom sensor channels for Atmel M90E26.
 */
enum m90e26_sensor_channel {
	/** Active and reactive energy accumulators, in Wh/VARh. */
	M90E26_SENSOR_CHANNEL_ENERGY = SENSOR_CHAN_PRIV_START + 1,
	/** Mean active, reactive and apparent power, in W/VAR/VA. */
	M90E26_SENSOR_CHANNEL_POWER,
	/** RMS line voltage, in V. */
	M90E26_SENSOR_CHANNEL_VOLTAGE,
	/** RMS line current, in A. */
	M90E26_SENSOR_CHANNEL_CURRENT,
	/** Line voltage frequency, in Hz. */
	M90E26_SENSOR_CHANNEL_FREQUENCY,
	/** Phase angle between voltage and current, in degrees. */
	M90E26_SENSOR_CHANNEL_PHASE_ANGLE,
	/** Power factor, unitless. */
	M90E26_SENSOR_CHANNEL_POWER_FACTOR,
};

/**
 * @brief Custom sensor trigger types for Atmel M90E26.
 */
enum m90e26_sensor_trigger_type {
	/** Active energy pulse output (CF1 pin). */
	M90E26_SENSOR_TRIG_TYPE_CF1 = SENSOR_TRIG_PRIV_START + 1,
	/** Reactive/apparent energy pulse output (CF2 pin). */
	M90E26_SENSOR_TRIG_TYPE_CF2,
	/** Interrupt output (IRQ pin). */
	M90E26_SENSOR_TRIG_TYPE_IRQ,
	/** Fatal error warning output (WRN_OUT pin). */
	M90E26_SENSOR_TRIG_TYPE_WRN_OUT,
};

/**
 * @brief m90e26_sensor_data Structure.
 *
 * This structure holds all the measurement data read from the M90E26 energy metering IC
 * converted to sensor_value format.
 */
struct m90e26_sensor_data {
	/** @brief Energy accumulator values, in Wh/VARh. */
	struct m90e26_energy_sensor_data {
		struct sensor_value APenergy; /**< Forward (imported) active energy. */
		struct sensor_value ANenergy; /**< Reverse (exported) active energy. */
		struct sensor_value ATenergy; /**< Total active energy. */
		struct sensor_value RPenergy; /**< Forward (inductive) reactive energy. */
		struct sensor_value RNenergy; /**< Reverse (capacitive) reactive energy. */
		struct sensor_value RTenergy; /**< Absolute reactive energy. */
	} energy_sensor_values;               /**< Energy accumulator values. */

	/** @brief Mean power values. */
	struct m90e26_power_sensor_data {
		struct sensor_value Pmean;  /**< L line mean active power, in W. */
		struct sensor_value Qmean;  /**< L line mean reactive power, in VAR. */
		struct sensor_value Smean;  /**< L line mean apparent power, in VA. */
		struct sensor_value Pmean2; /**< N line mean active power, in W. */
		struct sensor_value Qmean2; /**< N line mean reactive power, in VAR. */
		struct sensor_value Smean2; /**< N line mean apparent power, in VA. */
	} power_sensor_values;              /**< Mean power values. */

	struct sensor_value Urms; /**< RMS line voltage, in V. */

	/** @brief RMS current values. */
	struct m90e26_current_sensor_data {
		struct sensor_value Irms;  /**< L line RMS current, in A. */
		struct sensor_value Irms2; /**< N line RMS current, in A. */
	} current_sensor_values;           /**< RMS current values. */

	struct sensor_value Freq; /**< Line voltage frequency, in Hz. */

	/** @brief Phase angle values, in degrees. */
	struct m90e26_phase_angle_sensor_data {
		struct sensor_value Pangle;  /**< Phase angle between voltage and L line current. */
		struct sensor_value Pangle2; /**< Phase angle between voltage and N line current. */
	} phase_angle_sensor_values;         /**< Phase angle values. */

	/** @brief Power factor values, unitless. */
	struct m90e26_power_factor_sensor_data {
		struct sensor_value PowerF;  /**< L line power factor. */
		struct sensor_value PowerF2; /**< N line power factor. */
	} power_factor_sensor_values;        /**< Power factor values. */
};

/**
 * @brief m90e26_config_registers Structure.
 *
 * This structure holds the configuration registers of the M90E26 device.
 */
struct m90e26_config_registers {
	/* Special Registers */

	m90e26_data_value_t FuncEn;    /**< Function enable. */
	m90e26_data_value_t SagTh;     /**< Voltage sag threshold. */
	m90e26_data_value_t SmallPMod; /**< Small power mode. */

	/* Metering Calibration and Condiguration Registers */

	m90e26_data_value_t PLconstH; /**< High word of the PL_Constant. */
	m90e26_data_value_t PLconstL; /**< Low word of the PL_Constant. */
	m90e26_data_value_t Lgain;    /**< L line calibration gain. */
	m90e26_data_value_t Lphi;     /**< L line calibration angle. */
	m90e26_data_value_t Ngain;    /**< N line calibration gain. */
	m90e26_data_value_t Nphi;     /**< N line calibration angle. */
	m90e26_data_value_t PStartTh; /**< Active startup power threshold. */
	m90e26_data_value_t PNolTh;   /**< Active no-load power threshold. */
	m90e26_data_value_t QStartTh; /**< Reactive startup power threshold. */
	m90e26_data_value_t QNolTh;   /**< Reactive no-load power threshold. */
	m90e26_data_value_t MMode;    /**< Metering mode configuration. */

	/* Measurement Calibration Registers */

	m90e26_data_value_t Ugain;    /**< Voltage RMS gain. */
	m90e26_data_value_t IgainL;   /**< L line current RMS gain. */
	m90e26_data_value_t IgainN;   /**< N line current RMS gain. */
	m90e26_data_value_t Uoffset;  /**< Voltage offset. */
	m90e26_data_value_t IoffsetL; /**< L line current offset. */
	m90e26_data_value_t IoffsetN; /**< N line current offset. */
	m90e26_data_value_t PoffsetL; /**< L line active power offset. */
	m90e26_data_value_t QoffsetL; /**< L line reactive power offset. */
	m90e26_data_value_t PoffsetN; /**< N line active power offset. */
	m90e26_data_value_t QoffsetN; /**< N line reactive power offset. */
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_M90E26_H_ */
