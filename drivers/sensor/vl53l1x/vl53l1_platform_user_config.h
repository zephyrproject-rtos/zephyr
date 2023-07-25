/* vl53l1x_platform_user_config.h - Zephyr customization of ST vl53l1x library. */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _VL53L1_PLATFORM_USER_CONFIG_H_
#define _VL53L1_PLATFORM_USER_CONFIG_H_

#define    VL53L1_BYTES_PER_WORD              2
#define    VL53L1_BYTES_PER_DWORD             4

/* Define polling delays */
#define VL53L1_BOOT_COMPLETION_POLLING_TIMEOUT_MS     500
#define VL53L1_RANGE_COMPLETION_POLLING_TIMEOUT_MS   2000
#define VL53L1_TEST_COMPLETION_POLLING_TIMEOUT_MS   60000

#define VL53L1_POLLING_DELAY_MS                         1

/* Define LLD TuningParms Page Base Address
 * - Part of Patch_AddedTuningParms_11761
 */
#define VL53L1_TUNINGPARM_PUBLIC_PAGE_BASE_ADDRESS  0x8000
#define VL53L1_TUNINGPARM_PRIVATE_PAGE_BASE_ADDRESS 0xC000

#define VL53L1_GAIN_FACTOR__STANDARD_DEFAULT       0x0800
	/*!<  Default standard ranging gain correction factor
	 *	  1.11 format. 1.0 = 0x0800, 0.980 = 0x07D7
	 */

#define VL53L1_OFFSET_CAL_MIN_EFFECTIVE_SPADS  0x0500
	/*!< Lower Limit for the  MM1 effective SPAD count during offset
	 *	 calibration Format 8.8 0x0500 -> 5.0 effective SPADs
	 */

#define VL53L1_OFFSET_CAL_MAX_PRE_PEAK_RATE_MCPS   0x1900
	/*!< Max Limit for the pre range peak rate during offset
	 * calibration Format 9.7 0x1900 -> 50.0 Mcps.
	 * If larger then in pile up
	 */

#define VL53L1_OFFSET_CAL_MAX_SIGMA_MM             0x0040
	/*!< Max sigma estimate limit during offset calibration
	 *   Check applies to pre-range, mm1 and mm2 ranges
	 *	 Format 14.2 0x0040 -> 16.0mm.
	 */

#define VL53L1_MAX_USER_ZONES                 1
	/*!< Max number of user Zones - maximal limitation from
	 * FW stream divide - value of 254
	 */

#define VL53L1_MAX_RANGE_RESULTS              2
	/*!< Allocates storage for return and reference restults */


#define VL53L1_MAX_STRING_LENGTH 512

#endif  /* _VL53L1_PLATFORM_USER_CONFIG_H_ */
