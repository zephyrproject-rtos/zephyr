/*
 * Copyright (c) 2025, Linumiz GmbH
 * Copyright (c) 2026, Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief HYCON HY4245 fuel gauge specific properties
 *
 * Device specific fuel gauge properties of the HY4245, used with
 * fuel_gauge_get_prop() and fuel_gauge_set_prop().
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_HY4245_H_
#define ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_HY4245_H_

#include <stdbool.h>

#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief HY4245 fuel gauge specific properties
 * @defgroup fuel_gauge_hy4245 HY4245 fuel gauge properties
 * @ingroup fuel_gauge_interface
 * @{
 */

/**
 * @name ControlStatus() bits, HY4245_FUEL_GAUGE_CONTROL_STATUS
 * @{
 */
/** Battery voltage is high enough for data flash updates */
#define HY4245_CONTROL_STATUS_VOK       BIT(1)
/** Device is in SLEEP mode */
#define HY4245_CONTROL_STATUS_SLEEP     BIT(4)
/** FULLSLEEP mode requested */
#define HY4245_CONTROL_STATUS_FULLSLEEP BIT(5)
/** HIBERNATE mode requested */
#define HY4245_CONTROL_STATUS_HIBERNATE BIT(6)
/** Shutdown feature of the SE pin enabled */
#define HY4245_CONTROL_STATUS_SHUTDOWN  BIT(7)
/** 1-Wire interface active */
#define HY4245_CONTROL_STATUS_1WR       BIT(8)
/** Board offset calibration routine (calibration mode) active */
#define HY4245_CONTROL_STATUS_BCA       BIT(10)
/** Coulomb counter offset calibration routine active */
#define HY4245_CONTROL_STATUS_CCA       BIT(11)
/** Data or instruction flash checksum generated */
#define HY4245_CONTROL_STATUS_CSV       BIT(12)
/** Device is sealed */
#define HY4245_CONTROL_STATUS_SS        BIT(13)
/** Full access is sealed */
#define HY4245_CONTROL_STATUS_FAS       BIT(14)
/** SE pin is active */
#define HY4245_CONTROL_STATUS_SE        BIT(15)
/** @} */

/**
 * @name Flags() bits, HY4245_FUEL_GAUGE_FLAGS
 * @{
 */
/** Discharging */
#define HY4245_FLAGS_DSG       BIT(0)
/** Capacity depleted, RemainingCapacity() below the SOCF threshold */
#define HY4245_FLAGS_SOCF      BIT(1)
/** Capacity low, RemainingCapacity() below the SOC1 threshold */
#define HY4245_FLAGS_SOC1      BIT(2)
/** Battery inserted */
#define HY4245_FLAGS_BAT_DET   BIT(3)
/** Capacity learned since the last ClearLearned() */
#define HY4245_FLAGS_LRND      BIT(4)
/** QuickStart() in progress */
#define HY4245_FLAGS_QSTART    BIT(5)
/** End of discharge compensation in progress */
#define HY4245_FLAGS_EODCTAKEN BIT(6)
/** Open circuit voltage measurement taken */
#define HY4245_FLAGS_OCVTAKEN  BIT(7)
/** Charging */
#define HY4245_FLAGS_CHG       BIT(8)
/** Fully charged */
#define HY4245_FLAGS_FC        BIT(9)
/** Charge suspended because of the temperature */
#define HY4245_FLAGS_CHGSUSP   BIT(10)
/** Charge inhibited because of the temperature */
#define HY4245_FLAGS_XCHG      BIT(11)
/** Discharge over temperature */
#define HY4245_FLAGS_DOT       BIT(14)
/** Charge over temperature */
#define HY4245_FLAGS_COT       BIT(15)
/** @} */

/**
 * @name SafetyStatus() bits, HY4245_FUEL_GAUGE_SAFETY_STATUS
 * @{
 */
/** Over voltage */
#define HY4245_SAFETY_STATUS_OV  BIT(6)
/** Under voltage */
#define HY4245_SAFETY_STATUS_UV  BIT(7)
/** Charge over current */
#define HY4245_SAFETY_STATUS_COC BIT(12)
/** Discharge over current */
#define HY4245_SAFETY_STATUS_DOC BIT(13)
/** Charge over temperature */
#define HY4245_SAFETY_STATUS_COT BIT(14)
/** Discharge over temperature */
#define HY4245_SAFETY_STATUS_DOT BIT(15)
/** @} */

/**
 * @name OperationCfgA() bits, HY4245_FUEL_GAUGE_OPERATION_CONFIG_A
 * @{
 */
/** Temperature() is measured at the TS input instead of the internal sensor */
#define HY4245_OPERATION_CONFIG_A_TEMPS    BIT(0)
/** SE pin functions enabled */
#define HY4245_OPERATION_CONFIG_A_SE_EN    BIT(1)
/** SE pin is active low */
#define HY4245_OPERATION_CONFIG_A_SE_POL   BIT(3)
/** RelativeStateOfCharge() is held at 99 % until charge termination */
#define HY4245_OPERATION_CONFIG_A_RMFCC    BIT(4)
/** SLEEP mode enabled */
#define HY4245_OPERATION_CONFIG_A_SLEEP    BIT(5)
/** Updates when the battery impedance is learned at normal temperature */
#define HY4245_OPERATION_CONFIG_A_UPIMP_B  BIT(6)
/** Two cells in series */
#define HY4245_OPERATION_CONFIG_A_CELL0    BIT(8)
/** Higher current measurement resolution for small sense resistors */
#define HY4245_OPERATION_CONFIG_A_SMLSR    BIT(10)
/** VSS connected to SRP instead of SRN */
#define HY4245_OPERATION_CONFIG_A_GNDSEL   BIT(11)
/** Battery is not removable */
#define HY4245_OPERATION_CONFIG_A_NR       BIT(13)
/** Design capacity is updated from Qmax() during the first capacity learning */
#define HY4245_OPERATION_CONFIG_A_FACDC_EN BIT(14)
/** Data flash updates by the algorithm in operation enabled */
#define HY4245_OPERATION_CONFIG_A_UPD_EN   BIT(15)
/** @} */

/**
 * @brief HY4245 specific fuel gauge properties
 *
 * The value type used with each property is given in the description:
 * @c custom_bool and @c custom_uint refer to the members of
 * union fuel_gauge_prop_val.
 */
enum hy4245_fuel_gauge_prop {
	/**
	 * Data flash updates by the gauge algorithm (OperationCfgA().UPD_EN).
	 *
	 * get, @c custom_bool: true if the gauge updates its data flash with
	 * learned values while operating.
	 *
	 * set, @c custom_bool: true enables the updates if they are not
	 * enabled yet (Control() subcommand SetUPD_EN). Disabling is not
	 * supported by the gauge.
	 */
	HY4245_FUEL_GAUGE_FLASH_UPDATE_ENABLE = FUEL_GAUGE_CUSTOM_BEGIN,
	/**
	 * ControlStatus() register.
	 *
	 * get, @c custom_uint: raw 16 bit register value, see
	 * HY4245_CONTROL_STATUS_*.
	 */
	HY4245_FUEL_GAUGE_CONTROL_STATUS,
	/**
	 * OperationCfgA() register.
	 *
	 * get, @c custom_uint: raw 16 bit register value, see
	 * HY4245_OPERATION_CONFIG_A_*.
	 */
	HY4245_FUEL_GAUGE_OPERATION_CONFIG_A,
	/**
	 * Flags() register.
	 *
	 * get, @c custom_uint: raw 16 bit register value, see HY4245_FLAGS_*.
	 */
	HY4245_FUEL_GAUGE_FLAGS,
	/**
	 * SafetyStatus() register.
	 *
	 * get, @c custom_uint: raw 16 bit register value, see
	 * HY4245_SAFETY_STATUS_*.
	 */
	HY4245_FUEL_GAUGE_SAFETY_STATUS,
	/**
	 * Accumulated duration of over temperature events
	 * (Control() subcommand LifetimeOverTempDuration).
	 *
	 * get, @c custom_uint: time in minutes.
	 */
	HY4245_FUEL_GAUGE_LIFETIME_OVER_TEMPERATURE_MINS,
	/**
	 * Data flash checksum (Control() subcommand DFChecksum).
	 *
	 * get, @c custom_uint: 16 bit checksum over the data flash. Used to
	 * identify or verify the programmed configuration image. The gauge
	 * computes the checksum only while it is unsealed.
	 */
	HY4245_FUEL_GAUGE_DATA_FLASH_CHECKSUM,
	/**
	 * Data flash version (Control() subcommand DFVersion), the version
	 * field of the programmed configuration image.
	 *
	 * get, @c custom_uint: 16 bit version.
	 */
	HY4245_FUEL_GAUGE_DATA_FLASH_VERSION,
	/**
	 * Firmware (algorithm) version (Control() subcommand Version).
	 *
	 * get, @c custom_uint: 16 bit version.
	 */
	HY4245_FUEL_GAUGE_FIRMWARE_VERSION,
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_HY4245_H_ */
