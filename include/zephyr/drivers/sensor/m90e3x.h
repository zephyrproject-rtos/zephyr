/*
 * Copyright (c) 2025 Atmel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of Atmel M90E3X energy metering IC
 * @ingroup m90e3x_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_M90E3X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_M90E3X_H_

/**
 * @defgroup m90e3x_interface M90E3X Interface
 * @brief Documentation for Atmel M90E3X energy metering IC interface
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Type definition for M90E3X register addresses.
 */
typedef uint16_t m90e3x_register_t;

/**
 * @brief Union for holding the data values read from the M90E3X registers.
 */
typedef union m90e3x_data_value {
	int16_t int16;
	uint16_t uint16;
} m90e3x_data_value_t;

/**
 * @brief Custom sensor channels for Atmel M90E3X.
 */
enum m90e3x_sensor_channel {
	M90E3X_SENSOR_CHANNEL_ENERGY = SENSOR_CHAN_PRIV_START + 1,
	M90E3X_SENSOR_CHANNEL_FUNDAMENTAL_ENERGY,
	M90E3X_SENSOR_CHANNEL_HARMONIC_ENERGY,
	M90E3X_SENSOR_CHANNEL_POWER,
	M90E3X_SENSOR_CHANNEL_POWER_FACTOR,
	M90E3X_SENSOR_CHANNEL_FUNDAMENTAL_POWER,
	M90E3X_SENSOR_CHANNEL_HARMONIC_POWER,
	M90E3X_SENSOR_CHANNEL_VOLTAGE,
	M90E3X_SENSOR_CHANNEL_CURRENT,
	M90E3X_SENSOR_CHANNEL_PEAK,
	M90E3X_SENSOR_CHANNEL_FREQUENCY,
	M90E3X_SENSOR_CHANNEL_PHASE_ANGLE,
	M90E3X_SENSOR_CHANNEL_TEMPERATURE,
};

/**
 * @brief Custom sensor trigger types for Atmel M90E3X.
 */
enum m90e3x_sensor_trigger_type {
	M90E3X_SENSOR_TRIG_TYPE_CF1 = SENSOR_TRIG_PRIV_START + 1,
	M90E3X_SENSOR_TRIG_TYPE_CF2,
	M90E3X_SENSOR_TRIG_TYPE_CF3,
	M90E3X_SENSOR_TRIG_TYPE_CF4,
	M90E3X_SENSOR_TRIG_TYPE_IRQ0,
	M90E3X_SENSOR_TRIG_TYPE_IRQ1,
	M90E3X_SENSOR_TRIG_TYPE_WRN_OUT,
};

/**
 * @brief M90E3X Power Mode Enumeration.
 */
enum m90e3x_power_mode {
	M90E3X_IDLE = 0,
	M90E3X_DETECTION = 1,
	M90E3X_PARTIAL = 2,
	M90E3X_NORMAL = 3
};

struct m90e3x_sensor_data {

	/**
	 * @brief Sensor values for Energy measurements. Contains data for all foward/reverse,
	 * apparent/active/reactive energy values.
	 */

	struct m90e3x_energy_sensor_data {
		struct sensor_value APenergyT;
		struct sensor_value APenergyA;
		struct sensor_value APenergyB;
		struct sensor_value APenergyC;
		struct sensor_value ANenergyT;
		struct sensor_value ANenergyA;
		struct sensor_value ANenergyB;
		struct sensor_value ANenergyC;
		struct sensor_value RPenergyT;
		struct sensor_value RPenergyA;
		struct sensor_value RPenergyB;
		struct sensor_value RPenergyC;
		struct sensor_value RNenergyT;
		struct sensor_value RNenergyA;
		struct sensor_value RNenergyB;
		struct sensor_value RNenergyC;
		struct sensor_value SAenergyT;
		struct sensor_value SenergyA;
		struct sensor_value SenergyB;
		struct sensor_value SenergyC;
		struct sensor_value SVenergyT;
	} energy_sensor_values;

	/**
	 * @brief Sensor values for Fundamental Energy measurements. Contains data for all
	 * forward/reverse active fundamental energy values.
	 */

	struct m90e3x_fundamental_energy_sensor_data {
		struct sensor_value APenergyTF;
		struct sensor_value APenergyAF;
		struct sensor_value APenergyBF;
		struct sensor_value APenergyCF;
		struct sensor_value ANenergyTF;
		struct sensor_value ANenergyAF;
		struct sensor_value ANenergyBF;
		struct sensor_value ANenergyCF;

	} fundamental_energy_sensor_values;

	/**
	 * @brief Sensor values for Harmonic Energy measurements. Contains data for all
	 * forward/reverse active harmonic energy values.
	 */

	struct m90e3x_harmonic_energy_sensor_data {
		struct sensor_value APenergyTH;
		struct sensor_value APenergyAH;
		struct sensor_value APenergyBH;
		struct sensor_value APenergyCH;
		struct sensor_value ANenergyTH;
		struct sensor_value ANenergyAH;
		struct sensor_value ANenergyBH;
		struct sensor_value ANenergyCH;
	} harmonic_energy_sensor_values;

	/**
	 * @brief Sensor values for Power measurements. Contains data for total and per-phase
	 * active, reactive and apparent power values.
	 */

	struct m90e3x_power_sensor_data {
		struct sensor_value PmeanT; /* [W] */
		struct sensor_value PmeanA; /* [W] */
		struct sensor_value PmeanB; /* [W] */
		struct sensor_value PmeanC; /* [W] */
		struct sensor_value QmeanT; /* [VAR] */
		struct sensor_value QmeanA; /* [VAR] */
		struct sensor_value QmeanB; /* [VAR] */
		struct sensor_value QmeanC; /* [VAR] */
		struct sensor_value SmeanT; /* [VA] */
		struct sensor_value SmeanA; /* [VA] */
		struct sensor_value SmeanB; /* [VA] */
		struct sensor_value SmeanC; /* [VA] */
	} power_sensor_values;

	/**
	 * @brief Sensor values for Power Factor measurements. Contains data for total and
	 * per-phase power factor values.
	 */
	struct m90e3x_power_factor_sensor_data {
		struct sensor_value PFmeanT; /* [ ] */
		struct sensor_value PFmeanA; /* [ ] */
		struct sensor_value PFmeanB; /* [ ] */
		struct sensor_value PFmeanC; /* [ ] */
	} power_factor_values;

	/**
	 * @brief Sensor values for Fundamental Power measurements. Contains data for total and
	 * per-phase active fundamental power values.
	 */

	struct m90e3x_fundamental_power_sensor_data {
		struct sensor_value PmeanTF; /* [W] */
		struct sensor_value PmeanAF; /* [W] */
		struct sensor_value PmeanBF; /* [W] */
		struct sensor_value PmeanCF; /* [W] */
	} power_fundamental_sensor_values;

	/**
	 * @brief Sensor values for Harmonic Power measurements. Contains data for total and
	 * per-phase for active harmonic power values.
	 */

	struct m90e3x_harmonic_power_sensor_data {
		struct sensor_value PmeanTH; /* [W] */
		struct sensor_value PmeanAH; /* [W] */
		struct sensor_value PmeanBH; /* [W] */
		struct sensor_value PmeanCH; /* [W] */
	} harmonic_power_sensor_values;

	/**
	 * @brief Sensor values for Voltage RMS measurements. Contains voltage data for all
	 * three phases.
	 */

	struct m90e3x_voltage_rms_sensor_data {
		struct sensor_value UrmsA; /* [V] */
		struct sensor_value UrmsB; /* [V] */
		struct sensor_value UrmsC; /* [V] */
	} voltage_rms_sensor_values;

	/**
	 * @brief Sensor values for Current RMS measurements. Contains current data for all
	 * three phases and neutral line.
	 */

	struct m90e3x_current_rms_sensor_data {
		struct sensor_value IrmsN; /* [A] */
		struct sensor_value IrmsA; /* [A] */
		struct sensor_value IrmsB; /* [A] */
		struct sensor_value IrmsC; /* [A] */
	} current_rms_sensor_values;

	/**
	 * @brief Sensor values for THD (M90E36A) or Peak (M90E32AS) measurements.
	 *
	 * @note In M90E32AS there are peak voltage/current registers.
	 *
	 * In M90E36A there are THD voltage/current registers.
	 *
	 * This union allows to use the same data structure for both ICs.
	 */

	union {
		struct m90e32as_peak_sensor_data {
			struct sensor_value UpkA; /* [V] */
			struct sensor_value UpkB; /* [V] */
			struct sensor_value UpkC; /* [V] */
			struct sensor_value IpkA; /* [A] */
			struct sensor_value IpkB; /* [A] */
			struct sensor_value IpkC; /* [A] */
		} peak_sensor_values;
		struct m90e36a_thd_sensor_data {
			struct sensor_value THDNUA; /* [%] */
			struct sensor_value THDNUB; /* [%] */
			struct sensor_value THDNUC; /* [%] */
			struct sensor_value THDNIA; /* [%] */
			struct sensor_value THDNIB; /* [%] */
			struct sensor_value THDNIC; /* [%] */
		} thd_sensor_values;
	};

	struct m90e3x_phase_angle_sensor_data {
		struct sensor_value PAngleA; /* [degrees] */
		struct sensor_value PAngleB; /* [degrees] */
		struct sensor_value PAngleC; /* [degrees] */
		struct sensor_value UAngleA; /* [degrees] */
		struct sensor_value UAngleB; /* [degrees] */
		struct sensor_value UAngleC; /* [degrees] */
	} phase_angle_sensor_values;

	struct sensor_value Frequency;   /* [Hz] */
	struct sensor_value Temperature; /* [°C] */

	/**
	 * @note Harmonic Fourier Analysis registers are not included in this data structure
	 * due to their large size (32 orders for voltage and current per phase).
	 */
};

/**
 * @brief m90e32as_config_registers Structure.
 *
 * This structure holds the configuration registers of the M90E32AS device.
 */
struct m90e32as_config_registers {

	/* Status and Special Registers */

	m90e3x_data_value_t MeterEn;
	m90e3x_data_value_t ChannelMapI;
	m90e3x_data_value_t ChannelMapU;
	m90e3x_data_value_t SagPeakDetCfg;
	m90e3x_data_value_t OVthCfg;
	m90e3x_data_value_t ZXConfig;
	m90e3x_data_value_t SagTh;
	m90e3x_data_value_t PhaseLossTh;
	m90e3x_data_value_t InWarnTh;
	m90e3x_data_value_t OIth;
	m90e3x_data_value_t FreqLoTh;
	m90e3x_data_value_t FreqHiTh;
	m90e3x_data_value_t PMPwrCtrl;
	m90e3x_data_value_t IRQ0MergeCfg;

	/* Low Power Mode Registers */

	m90e3x_data_value_t DetectCtrl;
	m90e3x_data_value_t DetectTh1;
	m90e3x_data_value_t DetectTh2;
	m90e3x_data_value_t DetectTh3;
	m90e3x_data_value_t IDCoffsetA;
	m90e3x_data_value_t IDCoffsetB;
	m90e3x_data_value_t IDCoffsetC;
	m90e3x_data_value_t UDCoffsetA;
	m90e3x_data_value_t UDCoffsetB;
	m90e3x_data_value_t UDCoffsetC;
	m90e3x_data_value_t UGainTAB;
	m90e3x_data_value_t UGainTC;
	m90e3x_data_value_t PhiFreqComp;
	m90e3x_data_value_t LOGIrms0;
	m90e3x_data_value_t LOGIrms1;
	m90e3x_data_value_t F0;
	m90e3x_data_value_t T0;
	m90e3x_data_value_t PhiAIrms01;
	m90e3x_data_value_t PhiAIrms2;
	m90e3x_data_value_t GainAIrms01;
	m90e3x_data_value_t GainAIrms2;
	m90e3x_data_value_t PhiBIrms01;
	m90e3x_data_value_t PhiBIrms2;
	m90e3x_data_value_t GainBIrms01;
	m90e3x_data_value_t GainBIrms2;
	m90e3x_data_value_t PhiCIrms01;
	m90e3x_data_value_t PhiCIrms2;
	m90e3x_data_value_t GainCIrms01;
	m90e3x_data_value_t GainCIrms2;

	/* Configuration Registers */

	m90e3x_data_value_t PLconstH;
	m90e3x_data_value_t PLconstL;
	m90e3x_data_value_t MMode0;
	m90e3x_data_value_t MMode1;
	m90e3x_data_value_t PStartTh;
	m90e3x_data_value_t QStartTh;
	m90e3x_data_value_t SStartTh;
	m90e3x_data_value_t PPhaseTh;
	m90e3x_data_value_t QPhaseTh;
	m90e3x_data_value_t SPhaseTh;

	/* Calibration Registers */

	m90e3x_data_value_t PoffsetA;
	m90e3x_data_value_t QoffsetA;
	m90e3x_data_value_t PoffsetB;
	m90e3x_data_value_t QoffsetB;
	m90e3x_data_value_t PoffsetC;
	m90e3x_data_value_t QoffsetC;
	m90e3x_data_value_t PQGainA;
	m90e3x_data_value_t PhiA;
	m90e3x_data_value_t PQGainB;
	m90e3x_data_value_t PhiB;
	m90e3x_data_value_t PQGainC;
	m90e3x_data_value_t PhiC;

	/* Fundamental / Harmonic Energy Calibration Registers */

	m90e3x_data_value_t PoffsetAF;
	m90e3x_data_value_t PoffsetBF;
	m90e3x_data_value_t PoffsetCF;
	m90e3x_data_value_t PGainAF;
	m90e3x_data_value_t PGainBF;
	m90e3x_data_value_t PGainCF;

	/* Measurement Calibration Registers */

	m90e3x_data_value_t UgainA;
	m90e3x_data_value_t IgainA;
	m90e3x_data_value_t UoffsetA;
	m90e3x_data_value_t IoffsetA;
	m90e3x_data_value_t UgainB;
	m90e3x_data_value_t IgainB;
	m90e3x_data_value_t UoffsetB;
	m90e3x_data_value_t IoffsetB;
	m90e3x_data_value_t UgainC;
	m90e3x_data_value_t IgainC;
	m90e3x_data_value_t UoffsetC;
	m90e3x_data_value_t IoffsetC;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_M90E3X_H_ */
