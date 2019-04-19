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
 * @file  vl53l1_error_exceptions.h
 *
 * @brief EwokPlus25 LL Driver definitions for control of error handling in LL driver
 */

#ifndef _VL53L1_ERROR_EXCEPTIONS_H_
#define _VL53L1_ERROR_EXCEPTIONS_H_

#define IGNORE_DIVISION_BY_ZERO                                0

#define IGNORE_XTALK_EXTRACTION_NO_SAMPLE_FAIL                 0
#define IGNORE_XTALK_EXTRACTION_SIGMA_LIMIT_FAIL               0
#define IGNORE_XTALK_EXTRACTION_NO_SAMPLE_FOR_GRADIENT_WARN    0
#define IGNORE_XTALK_EXTRACTION_SIGMA_LIMIT_FOR_GRADIENT_WARN  0
#define IGNORE_XTALK_EXTRACTION_MISSING_SAMPLES_WARN           0

#define IGNORE_REF_SPAD_CHAR_NOT_ENOUGH_SPADS                  0
#define IGNORE_REF_SPAD_CHAR_RATE_TOO_HIGH                     0
#define IGNORE_REF_SPAD_CHAR_RATE_TOO_LOW                      0

#define IGNORE_OFFSET_CAL_MISSING_SAMPLES                      0
#define IGNORE_OFFSET_CAL_SIGMA_TOO_HIGH                       0
#define IGNORE_OFFSET_CAL_RATE_TOO_HIGH                        0
#define IGNORE_OFFSET_CAL_SPAD_COUNT_TOO_LOW				   0

#define IGNORE_ZONE_CAL_MISSING_SAMPLES                        0
#define IGNORE_ZONE_CAL_SIGMA_TOO_HIGH                         0
#define IGNORE_ZONE_CAL_RATE_TOO_HIGH                          0

#endif /* _VL53L1_ERROR_EXCEPTIONS_H_ */
