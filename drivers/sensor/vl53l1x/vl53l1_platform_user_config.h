/*
* Copyright (c) 2017, STMicroelectronics - All Rights Reserved
*
* This file is part of VL53L1 Core and is dual licensed,
* either 'STMicroelectronics
* Proprietary license'
* or 'BSD 3-clause "New" or "Revised" License' , at your option.
*
********************************************************************************
*
* 'STMicroelectronics Proprietary license'
*
********************************************************************************
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0081
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*
********************************************************************************
*
* Alternatively, VL53L1 Core may be distributed under the terms of
* 'BSD 3-clause "New" or "Revised" License', in which case the following
* provisions apply instead of the ones mentioned above :
*
********************************************************************************
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
********************************************************************************
*
*/

/**
 * @file  vl53l1_platform_user_config.h
 *
 * @brief EwokPlus compile time user modifiable configuration
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
	      1.11 format. 1.0 = 0x0800, 0.980 = 0x07D7 */

#define VL53L1_OFFSET_CAL_MIN_EFFECTIVE_SPADS  0x0500
	/*!< Lower Limit for the  MM1 effective SPAD count during offset
	     calibration Format 8.8 0x0500 -> 5.0 effective SPADs */

#define VL53L1_OFFSET_CAL_MAX_PRE_PEAK_RATE_MCPS   0x1900
	/*!< Max Limit for the pre range peak rate during offset
	     calibration Format 9.7 0x1900 -> 50.0 Mcps.
	     If larger then in pile up */

#define VL53L1_OFFSET_CAL_MAX_SIGMA_MM             0x0040
	/*!< Max sigma estimate limit during offset calibration
	     Check applies to pre-range, mm1 and mm2 ranges
	     Format 14.2 0x0040 -> 16.0mm. */

#define VL53L1_MAX_USER_ZONES                 169
	/*!< Max number of user Zones - maximal limitation from
		 FW stream divide - value of 254 */

#define VL53L1_MAX_RANGE_RESULTS              2
	/*!< Allocates storage for return and reference restults */


#define VL53L1_MAX_STRING_LENGTH 512

#endif  /* _VL53L1_PLATFORM_USER_CONFIG_H_ */

