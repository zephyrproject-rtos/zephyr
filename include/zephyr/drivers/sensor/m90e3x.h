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
 * @ingroup sensor_interface_ext_atmel
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
	int16_t int16;   /**< Value interpreted as a signed 16-bit integer. */
	uint16_t uint16; /**< Value interpreted as an unsigned 16-bit integer. */
} m90e3x_data_value_t;

/**
 * @brief Custom sensor channels for Atmel M90E3X.
 */
enum m90e3x_sensor_channel {
	/** Active, reactive and apparent energy accumulators, in Wh/VARh/VAh. */
	M90E3X_SENSOR_CHANNEL_ENERGY = SENSOR_CHAN_PRIV_START + 1,
	/** Fundamental (line-frequency) active energy accumulators, in Wh. */
	M90E3X_SENSOR_CHANNEL_FUNDAMENTAL_ENERGY,
	/** Harmonic active energy accumulators, in Wh. */
	M90E3X_SENSOR_CHANNEL_HARMONIC_ENERGY,
	/** Mean active, reactive and apparent power, in W/VAR/VA. */
	M90E3X_SENSOR_CHANNEL_POWER,
	/** Power factor, unitless. */
	M90E3X_SENSOR_CHANNEL_POWER_FACTOR,
	/** Fundamental (line-frequency) active power, in W. */
	M90E3X_SENSOR_CHANNEL_FUNDAMENTAL_POWER,
	/** Harmonic active power, in W. */
	M90E3X_SENSOR_CHANNEL_HARMONIC_POWER,
	/** RMS line voltage, in V. */
	M90E3X_SENSOR_CHANNEL_VOLTAGE,
	/** RMS line current, in A. */
	M90E3X_SENSOR_CHANNEL_CURRENT,
	/** Peak voltage/current (M90E32AS) or THD (M90E36A) values. */
	M90E3X_SENSOR_CHANNEL_PEAK,
	/** Line voltage frequency, in Hz. */
	M90E3X_SENSOR_CHANNEL_FREQUENCY,
	/** Phase angle between voltage and current, in degrees. */
	M90E3X_SENSOR_CHANNEL_PHASE_ANGLE,
	/** Measured chip temperature, in °C. */
	M90E3X_SENSOR_CHANNEL_TEMPERATURE,
};

/**
 * @brief Custom sensor trigger types for Atmel M90E3X.
 */
enum m90e3x_sensor_trigger_type {
	/** Active energy pulse output (CF1 pin). */
	M90E3X_SENSOR_TRIG_TYPE_CF1 = SENSOR_TRIG_PRIV_START + 1,
	/** Reactive/apparent energy pulse output (CF2 pin). */
	M90E3X_SENSOR_TRIG_TYPE_CF2,
	/** Active fundamental energy pulse output (CF3 pin). */
	M90E3X_SENSOR_TRIG_TYPE_CF3,
	/** Active harmonic energy pulse output (CF4 pin). */
	M90E3X_SENSOR_TRIG_TYPE_CF4,
	/** Interrupt output 0 (IRQ0 pin). */
	M90E3X_SENSOR_TRIG_TYPE_IRQ0,
	/** Interrupt output 1 (IRQ1 pin). */
	M90E3X_SENSOR_TRIG_TYPE_IRQ1,
	/** Fatal error warning output (WRN_OUT pin). */
	M90E3X_SENSOR_TRIG_TYPE_WRN_OUT,
};

/**
 * @brief M90E3X Power Mode Enumeration.
 */
enum m90e3x_power_mode {
	/** Lowest-power mode; metering is stopped. */
	M90E3X_IDLE = 0,
	/** Low-power mode that only compares current against a threshold to detect a load. */
	M90E3X_DETECTION = 1,
	/** Partial measurement mode; only a reduced subset of channels stays active. */
	M90E3X_PARTIAL = 2,
	/** Full metering mode with all measurement channels active. */
	M90E3X_NORMAL = 3
};

/**
 * @brief m90e3x_sensor_data Structure.
 *
 * This structure holds all the measurement data read from an M90E32AS or M90E36A energy
 * metering IC converted to sensor_value format.
 */
struct m90e3x_sensor_data {

	/**
	 * @brief Sensor values for Energy measurements. Contains data for all foward/reverse,
	 * apparent/active/reactive energy values.
	 */

	struct m90e3x_energy_sensor_data {
		struct sensor_value APenergyT; /**< Total forward active energy. */
		struct sensor_value APenergyA; /**< Phase A forward active energy. */
		struct sensor_value APenergyB; /**< Phase B forward active energy. */
		struct sensor_value APenergyC; /**< Phase C forward active energy. */
		struct sensor_value ANenergyT; /**< Total reverse active energy. */
		struct sensor_value ANenergyA; /**< Phase A reverse active energy. */
		struct sensor_value ANenergyB; /**< Phase B reverse active energy. */
		struct sensor_value ANenergyC; /**< Phase C reverse active energy. */
		struct sensor_value RPenergyT; /**< Total forward reactive energy. */
		struct sensor_value RPenergyA; /**< Phase A forward reactive energy. */
		struct sensor_value RPenergyB; /**< Phase B forward reactive energy. */
		struct sensor_value RPenergyC; /**< Phase C forward reactive energy. */
		struct sensor_value RNenergyT; /**< Total reverse reactive energy. */
		struct sensor_value RNenergyA; /**< Phase A reverse reactive energy. */
		struct sensor_value RNenergyB; /**< Phase B reverse reactive energy. */
		struct sensor_value RNenergyC; /**< Phase C reverse reactive energy. */
		struct sensor_value SAenergyT; /**< Total apparent energy (arithmetic sum). */
		struct sensor_value SenergyA;  /**< Phase A apparent energy. */
		struct sensor_value SenergyB;  /**< Phase B apparent energy. */
		struct sensor_value SenergyC;  /**< Phase C apparent energy. */
		struct sensor_value SVenergyT; /**< Total apparent energy (vector sum, M90E36A). */
	} energy_sensor_values;                /**< Sensor values for energy measurements. */

	/**
	 * @brief Sensor values for Fundamental Energy measurements. Contains data for all
	 * forward/reverse active fundamental energy values.
	 */

	struct m90e3x_fundamental_energy_sensor_data {
		struct sensor_value APenergyTF; /**< Total forward active fundamental energy. */
		struct sensor_value APenergyAF; /**< Phase A forward active fundamental energy. */
		struct sensor_value APenergyBF; /**< Phase B forward active fundamental energy. */
		struct sensor_value APenergyCF; /**< Phase C forward active fundamental energy. */
		struct sensor_value ANenergyTF; /**< Total reverse active fundamental energy. */
		struct sensor_value ANenergyAF; /**< Phase A reverse active fundamental energy. */
		struct sensor_value ANenergyBF; /**< Phase B reverse active fundamental energy. */
		struct sensor_value ANenergyCF; /**< Phase C reverse active fundamental energy. */

	} fundamental_energy_sensor_values; /**< Fundamental energy sensor values. */

	/**
	 * @brief Sensor values for Harmonic Energy measurements. Contains data for all
	 * forward/reverse active harmonic energy values.
	 */

	struct m90e3x_harmonic_energy_sensor_data {
		struct sensor_value APenergyTH; /**< Total forward active harmonic energy. */
		struct sensor_value APenergyAH; /**< Phase A forward active harmonic energy. */
		struct sensor_value APenergyBH; /**< Phase B forward active harmonic energy. */
		struct sensor_value APenergyCH; /**< Phase C forward active harmonic energy. */
		struct sensor_value ANenergyTH; /**< Total reverse active harmonic energy. */
		struct sensor_value ANenergyAH; /**< Phase A reverse active harmonic energy. */
		struct sensor_value ANenergyBH; /**< Phase B reverse active harmonic energy. */
		struct sensor_value ANenergyCH; /**< Phase C reverse active harmonic energy. */
	} harmonic_energy_sensor_values; /**< Sensor values for harmonic energy measurements. */

	/**
	 * @brief Sensor values for Power measurements. Contains data for total and per-phase
	 * active, reactive and apparent power values.
	 */

	struct m90e3x_power_sensor_data {
		struct sensor_value PmeanT; /**< Total active power, in W. */
		struct sensor_value PmeanA; /**< Phase A active power, in W. */
		struct sensor_value PmeanB; /**< Phase B active power, in W. */
		struct sensor_value PmeanC; /**< Phase C active power, in W. */
		struct sensor_value QmeanT; /**< Total reactive power, in VAR. */
		struct sensor_value QmeanA; /**< Phase A reactive power, in VAR. */
		struct sensor_value QmeanB; /**< Phase B reactive power, in VAR. */
		struct sensor_value QmeanC; /**< Phase C reactive power, in VAR. */
		struct sensor_value SmeanT; /**< Total apparent power, in VA. */
		struct sensor_value SmeanA; /**< Phase A apparent power, in VA. */
		struct sensor_value SmeanB; /**< Phase B apparent power, in VA. */
		struct sensor_value SmeanC; /**< Phase C apparent power, in VA. */
	} power_sensor_values;              /**< Sensor values for power measurements. */

	/**
	 * @brief Sensor values for Power Factor measurements. Contains data for total and
	 * per-phase power factor values.
	 */
	struct m90e3x_power_factor_sensor_data {
		struct sensor_value PFmeanT; /**< Total power factor. */
		struct sensor_value PFmeanA; /**< Phase A power factor. */
		struct sensor_value PFmeanB; /**< Phase B power factor. */
		struct sensor_value PFmeanC; /**< Phase C power factor. */
	} power_factor_values;               /**< Sensor values for power factor measurements. */

	/**
	 * @brief Sensor values for Fundamental Power measurements. Contains data for total and
	 * per-phase active fundamental power values.
	 */

	struct m90e3x_fundamental_power_sensor_data {
		struct sensor_value PmeanTF; /**< Total active fundamental power, in W. */
		struct sensor_value PmeanAF; /**< Phase A active fundamental power, in W. */
		struct sensor_value PmeanBF; /**< Phase B active fundamental power, in W. */
		struct sensor_value PmeanCF; /**< Phase C active fundamental power, in W. */
	} power_fundamental_sensor_values; /**< Sensor values for fundamental power measurements. */

	/**
	 * @brief Sensor values for Harmonic Power measurements. Contains data for total and
	 * per-phase for active harmonic power values.
	 */

	struct m90e3x_harmonic_power_sensor_data {
		struct sensor_value PmeanTH; /**< Total active harmonic power, in W. */
		struct sensor_value PmeanAH; /**< Phase A active harmonic power, in W. */
		struct sensor_value PmeanBH; /**< Phase B active harmonic power, in W. */
		struct sensor_value PmeanCH; /**< Phase C active harmonic power, in W. */
	} harmonic_power_sensor_values;      /**< Sensor values for harmonic power measurements. */

	/**
	 * @brief Sensor values for Voltage RMS measurements. Contains voltage data for all
	 * three phases.
	 */

	struct m90e3x_voltage_rms_sensor_data {
		struct sensor_value UrmsA; /**< Phase A RMS voltage, in V. */
		struct sensor_value UrmsB; /**< Phase B RMS voltage, in V. */
		struct sensor_value UrmsC; /**< Phase C RMS voltage, in V. */
	} voltage_rms_sensor_values;       /**< Sensor values for voltage RMS measurements. */

	/**
	 * @brief Sensor values for Current RMS measurements. Contains current data for all
	 * three phases and neutral line.
	 */

	struct m90e3x_current_rms_sensor_data {
		struct sensor_value IrmsN; /**< Neutral line RMS current, in A. */
		struct sensor_value IrmsA; /**< Phase A RMS current, in A. */
		struct sensor_value IrmsB; /**< Phase B RMS current, in A. */
		struct sensor_value IrmsC; /**< Phase C RMS current, in A. */
	} current_rms_sensor_values;       /**< Sensor values for current RMS measurements. */

	/**
	 * @brief Sensor values for THD (M90E36A) or Peak (M90E32AS) measurements.
	 *
	 * @note In M90E32AS there are peak voltage/current registers.
	 *
	 * In M90E36A there are THD voltage/current registers.
	 *
	 * This union allows to use the same data structure for both ICs.
	 */

	union common_section {
		/** @brief Peak voltage/current values (M90E32AS only). */
		struct m90e32as_peak_sensor_data {
			struct sensor_value UpkA; /**< Phase A peak voltage, in V. */
			struct sensor_value UpkB; /**< Phase B peak voltage, in V. */
			struct sensor_value UpkC; /**< Phase C peak voltage, in V. */
			struct sensor_value IpkA; /**< Phase A peak current, in A. */
			struct sensor_value IpkB; /**< Phase B peak current, in A. */
			struct sensor_value IpkC; /**< Phase C peak current, in A. */
		} peak_sensor_values;             /**< Peak voltage/current values. */
		/** @brief Total harmonic distortion values (M90E36A only). */
		struct m90e36a_thd_sensor_data {
			struct sensor_value THDNUA; /**< Phase A voltage THD, in %. */
			struct sensor_value THDNUB; /**< Phase B voltage THD, in %. */
			struct sensor_value THDNUC; /**< Phase C voltage THD, in %. */
			struct sensor_value THDNIA; /**< Phase A current THD, in %. */
			struct sensor_value THDNIB; /**< Phase B current THD, in %. */
			struct sensor_value THDNIC; /**< Phase C current THD, in %. */
		} thd_sensor_values;                /**< Total harmonic distortion values. */
	} common; /**< Peak (M90E32AS) or THD (M90E36A) sensor values. */

	/** @brief Phase angle values, in degrees. */
	struct m90e3x_phase_angle_sensor_data {
		struct sensor_value PAngleA; /**< Phase A voltage-to-current phase angle. */
		struct sensor_value PAngleB; /**< Phase B voltage-to-current phase angle. */
		struct sensor_value PAngleC; /**< Phase C voltage-to-current phase angle. */
		struct sensor_value UAngleA; /**< Phase A voltage phase angle. */
		struct sensor_value UAngleB; /**< Phase B voltage phase angle. */
		struct sensor_value UAngleC; /**< Phase C voltage phase angle. */
	} phase_angle_sensor_values;         /**< Phase angle values. */

	struct sensor_value Frequency;   /**< Line voltage frequency, in Hz. */
	struct sensor_value Temperature; /**< Measured chip temperature, in °C. */

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

	m90e3x_data_value_t MeterEn;       /**< Metering enable. */
	m90e3x_data_value_t ChannelMapI;   /**< Current channel mapping configuration. */
	m90e3x_data_value_t ChannelMapU;   /**< Voltage channel mapping configuration. */
	m90e3x_data_value_t SagPeakDetCfg; /**< Sag and peak detector period configuration. */
	m90e3x_data_value_t OVthCfg;       /**< Over voltage threshold. */
	m90e3x_data_value_t ZXConfig;      /**< Zero-crossing configuration for the ZX0/1/2 pins. */
	m90e3x_data_value_t SagTh;         /**< Voltage sag threshold. */
	m90e3x_data_value_t PhaseLossTh;   /**< Voltage phase losing threshold. */
	m90e3x_data_value_t InWarnTh;      /**< Calculated neutral current warning threshold. */
	m90e3x_data_value_t OIth;          /**< Over current threshold. */
	m90e3x_data_value_t FreqLoTh;      /**< Low threshold for frequency detection. */
	m90e3x_data_value_t FreqHiTh;      /**< High threshold for frequency detection. */
	m90e3x_data_value_t PMPwrCtrl;     /**< Partial measurement mode power control. */
	m90e3x_data_value_t IRQ0MergeCfg;  /**< IRQ0 merge configuration. */

	/* Low Power Mode Registers */

	m90e3x_data_value_t DetectCtrl; /**< Current detect control. */
	m90e3x_data_value_t DetectTh1;  /**< Channel 1 current threshold in detection mode. */
	m90e3x_data_value_t DetectTh2;  /**< Channel 2 current threshold in detection mode. */
	m90e3x_data_value_t DetectTh3;  /**< Channel 3 current threshold in detection mode. */
	m90e3x_data_value_t IDCoffsetA; /**< Phase A current DC offset. */
	m90e3x_data_value_t IDCoffsetB; /**< Phase B current DC offset. */
	m90e3x_data_value_t IDCoffsetC; /**< Phase C current DC offset. */
	m90e3x_data_value_t UDCoffsetA; /**< Phase A voltage DC offset. */
	m90e3x_data_value_t UDCoffsetB; /**< Phase B voltage DC offset. */
	m90e3x_data_value_t UDCoffsetC; /**< Phase C voltage DC offset. */
	m90e3x_data_value_t UGainTAB;   /**< Voltage gain temperature compensation for phase A/B. */
	m90e3x_data_value_t UGainTC;    /**< Voltage gain temperature compensation for phase C. */
	m90e3x_data_value_t PhiFreqComp; /**< Phase compensation for frequency. */
	m90e3x_data_value_t LOGIrms0;    /**< Log(Irms0) segment-compensation configuration. */
	m90e3x_data_value_t LOGIrms1;    /**< Log(Irms1) segment-compensation configuration. */
	m90e3x_data_value_t F0;          /**< Nominal frequency. */
	m90e3x_data_value_t T0;          /**< Nominal temperature. */
	m90e3x_data_value_t PhiAIrms01; /**< Phase A phase compensation for current segments 0/1. */
	m90e3x_data_value_t PhiAIrms2;  /**< Phase A phase compensation for current segment 2. */
	m90e3x_data_value_t GainAIrms01; /**< Phase A gain compensation for current segments 0/1. */
	m90e3x_data_value_t GainAIrms2;  /**< Phase A gain compensation for current segment 2. */
	m90e3x_data_value_t PhiBIrms01; /**< Phase B phase compensation for current segments 0/1. */
	m90e3x_data_value_t PhiBIrms2;  /**< Phase B phase compensation for current segment 2. */
	m90e3x_data_value_t GainBIrms01; /**< Phase B gain compensation for current segments 0/1. */
	m90e3x_data_value_t GainBIrms2;  /**< Phase B gain compensation for current segment 2. */
	m90e3x_data_value_t PhiCIrms01; /**< Phase C phase compensation for current segments 0/1. */
	m90e3x_data_value_t PhiCIrms2;  /**< Phase C phase compensation for current segment 2. */
	m90e3x_data_value_t GainCIrms01; /**< Phase C gain compensation for current segments 0/1. */
	m90e3x_data_value_t GainCIrms2;  /**< Phase C gain compensation for current segment 2. */

	/* Configuration Registers */

	m90e3x_data_value_t PLconstH; /**< High word of the PL_Constant. */
	m90e3x_data_value_t PLconstL; /**< Low word of the PL_Constant. */
	m90e3x_data_value_t MMode0;   /**< Metering method configuration. */
	m90e3x_data_value_t MMode1;   /**< PGA gain configuration. */
	m90e3x_data_value_t PStartTh; /**< Active startup power threshold. */
	m90e3x_data_value_t QStartTh; /**< Reactive startup power threshold. */
	m90e3x_data_value_t SStartTh; /**< Apparent startup power threshold. */
	m90e3x_data_value_t PPhaseTh; /**< Active energy accumulation threshold for any phase. */
	m90e3x_data_value_t QPhaseTh; /**< Reactive energy accumulation threshold for any phase. */
	m90e3x_data_value_t SPhaseTh; /**< Apparent energy accumulation threshold for any phase. */

	/* Calibration Registers */

	m90e3x_data_value_t PoffsetA; /**< Phase A active power offset. */
	m90e3x_data_value_t QoffsetA; /**< Phase A reactive power offset. */
	m90e3x_data_value_t PoffsetB; /**< Phase B active power offset. */
	m90e3x_data_value_t QoffsetB; /**< Phase B reactive power offset. */
	m90e3x_data_value_t PoffsetC; /**< Phase C active power offset. */
	m90e3x_data_value_t QoffsetC; /**< Phase C reactive power offset. */
	m90e3x_data_value_t PQGainA;  /**< Phase A calibration gain. */
	m90e3x_data_value_t PhiA;     /**< Phase A calibration phase angle. */
	m90e3x_data_value_t PQGainB;  /**< Phase B calibration gain. */
	m90e3x_data_value_t PhiB;     /**< Phase B calibration phase angle. */
	m90e3x_data_value_t PQGainC;  /**< Phase C calibration gain. */
	m90e3x_data_value_t PhiC;     /**< Phase C calibration phase angle. */

	/* Fundamental / Harmonic Energy Calibration Registers */

	m90e3x_data_value_t PoffsetAF; /**< Phase A fundamental active power offset. */
	m90e3x_data_value_t PoffsetBF; /**< Phase B fundamental active power offset. */
	m90e3x_data_value_t PoffsetCF; /**< Phase C fundamental active power offset. */
	m90e3x_data_value_t PGainAF;   /**< Phase A fundamental calibration gain. */
	m90e3x_data_value_t PGainBF;   /**< Phase B fundamental calibration gain. */
	m90e3x_data_value_t PGainCF;   /**< Phase C fundamental calibration gain. */

	/* Measurement Calibration Registers */

	m90e3x_data_value_t UgainA;   /**< Phase A voltage RMS gain. */
	m90e3x_data_value_t IgainA;   /**< Phase A current RMS gain. */
	m90e3x_data_value_t UoffsetA; /**< Phase A voltage RMS offset. */
	m90e3x_data_value_t IoffsetA; /**< Phase A current RMS offset. */
	m90e3x_data_value_t UgainB;   /**< Phase B voltage RMS gain. */
	m90e3x_data_value_t IgainB;   /**< Phase B current RMS gain. */
	m90e3x_data_value_t UoffsetB; /**< Phase B voltage RMS offset. */
	m90e3x_data_value_t IoffsetB; /**< Phase B current RMS offset. */
	m90e3x_data_value_t UgainC;   /**< Phase C voltage RMS gain. */
	m90e3x_data_value_t IgainC;   /**< Phase C current RMS gain. */
	m90e3x_data_value_t UoffsetC; /**< Phase C voltage RMS offset. */
	m90e3x_data_value_t IoffsetC; /**< Phase C current RMS offset. */
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_M90E3X_H_ */
