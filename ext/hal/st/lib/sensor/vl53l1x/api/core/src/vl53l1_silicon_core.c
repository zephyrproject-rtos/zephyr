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
 * @file  vl53l1_silicon_core.c
 *
 * @brief EwokPlus25 low level silicon LL Driver function definition
 */


#include "vl53l1_ll_def.h"
#include "vl53l1_platform.h"
#include "vl53l1_register_map.h"
#include "vl53l1_core.h"
#include "vl53l1_silicon_core.h"


#define LOG_FUNCTION_START(fmt, ...) \
	_LOG_FUNCTION_START(VL53L1_TRACE_MODULE_CORE, fmt, ##__VA_ARGS__)
#define LOG_FUNCTION_END(status, ...) \
	_LOG_FUNCTION_END(VL53L1_TRACE_MODULE_CORE, status, ##__VA_ARGS__)
#define LOG_FUNCTION_END_FMT(status, fmt, ...) \
	_LOG_FUNCTION_END_FMT(VL53L1_TRACE_MODULE_CORE, status, fmt, ##__VA_ARGS__)


VL53L1_Error VL53L1_is_firmware_ready_silicon(
	VL53L1_DEV     Dev,
	uint8_t       *pready)
{
	/**
	 * Determines if the firmware is ready to range
	 *
	 * There are 2 different behaviors depending on whether
	 * power force is enabled or not
	 */

	VL53L1_Error status = VL53L1_ERROR_NONE;
	VL53L1_LLDriverData_t *pdev = VL53L1DevStructGetLLDriverHandle(Dev);

	uint8_t  comms_buffer[5];

	LOG_FUNCTION_START("");

	/* read interrupt and power force reset status */

	status = VL53L1_ReadMulti(
					Dev,
					VL53L1_INTERRUPT_MANAGER__ENABLES,
					comms_buffer,
					5);

	if (status == VL53L1_ERROR_NONE) {

		pdev->dbg_results.interrupt_manager__enables =
				comms_buffer[0];
		pdev->dbg_results.interrupt_manager__clear =
				comms_buffer[1];
		pdev->dbg_results.interrupt_manager__status =
				comms_buffer[2];
		pdev->dbg_results.mcu_to_host_bank__wr_access_en =
				comms_buffer[3];
		pdev->dbg_results.power_management__go1_reset_status =
				comms_buffer[4];

		if ((pdev->sys_ctrl.power_management__go1_power_force & 0x01) == 0x01) {

				if (((pdev->dbg_results.interrupt_manager__enables & 0x1F) == 0x1F) &&
					((pdev->dbg_results.interrupt_manager__clear   & 0x1F) == 0x1F))
					*pready = 0x01;
				else
					*pready = 0x00;

		} else {

			/* set ready flag if bit 0 is zero i.g G01 is in reset */
			if ((pdev->dbg_results.power_management__go1_reset_status & 0x01) == 0x00)
				*pready = 0x01;
			else
				*pready = 0x00;
		}

	}

	LOG_FUNCTION_END(status);

	return status;
}
