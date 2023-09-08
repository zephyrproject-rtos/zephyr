/*
 * Copyright 2023 Google LLC
 * Copyright 2023 Microsoft Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_SBS_H_
#define ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_SBS_H_

/**
 * @brief Fuel Gauge Interface
 * @defgroup fuel_gauge_interface Fuel Gauge Interface
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <zephyr/drivers/fuel_gauge.h>

/**
 * Data structures for reading SBS buffer properties
 */
#define SBS_GAUGE_MANUFACTURER_NAME_MAX_SIZE 20
#define SBS_GAUGE_DEVICE_NAME_MAX_SIZE       20
#define SBS_GAUGE_DEVICE_CHEMISTRY_MAX_SIZE  4

struct sbs_gauge_manufacturer_name {
	uint8_t manufacturer_name_length;
	char manufacturer_name[SBS_GAUGE_MANUFACTURER_NAME_MAX_SIZE];
} __packed;

struct sbs_gauge_device_name {
	uint8_t device_name_length;
	char device_name[SBS_GAUGE_DEVICE_NAME_MAX_SIZE];
} __packed;

struct sbs_gauge_device_chemistry {
	uint8_t device_chemistry_length;
	char device_chemistry[SBS_GAUGE_DEVICE_CHEMISTRY_MAX_SIZE];
} __packed;

enum fuel_gauge_sbs_property {
	/** Denotes start of SBS specific properties */
	FUEL_GAUGE_SBS_PROPS_START = _FUEL_GAUGE_SBS_START,
	/** Retrieve word from SBS1.1 ManufactuerAccess */
	FUEL_GAUGE_SBS_MFR_ACCESS,
	/** Battery Mode (flags) */
	FUEL_GAUGE_SBS_MODE,
	/** AtRate (mA or 10 mW) */
	FUEL_GAUGE_SBS_ATRATE,
	/** AtRateTimeToFull (minutes) */
	FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL,
	/** AtRateTimeToEmpty (minutes) */
	FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY,
	/** AtRateOK (boolean) */
	FUEL_GAUGE_SBS_ATRATE_OK,
	/** Remaining Capacity Alarm (mAh or 10mWh) */
	FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM,
	/** Remaining Time Alarm (minutes) */
	FUEL_GAUGE_SBS_REMAINING_TIME_ALARM,
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_SBS_H_ */
