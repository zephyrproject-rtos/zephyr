/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _DBG_RAM_CAPTURE_H_
/* clang-format off */
#define _DBG_RAM_CAPTURE_H_
/* clang-format on */

#include "fsl_common.h"
#include "fsl_device_registers.h"

/*!
 * @addtogroup xcvr
 * @{
 */

/*! @file*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Page definitions */
#define DBG_PAGE_IDLE           (0x00)
#define DBG_PAGE_RXDIGIQ        (0x01)
#define DBG_PAGE_RAWADCIQ       (0x04)
#define DBG_PAGE_DCESTIQ        (0x07)
#define DBG_PAGE_RXINPH         (0x0A)
#define DBG_PAGE_DEMOD_HARD     (0x0B)
#define DBG_PAGE_DEMOD_SOFT     (0x0C)
#define DBG_PAGE_DEMOD_DATA     (0x0D)
#define DBG_PAGE_DEMOD_CFO_PH   (0x0E)

typedef enum _dbgRamStatus
{
  DBG_RAM_SUCCESS = 0,
  DBG_RAM_FAIL_SAMPLE_NUM_LIMIT = 1,
  DBG_RAM_FAIL_PAGE_ERROR = 2,
  DBG_RAM_FAIL_NULL_POINTER = 3,
} dbgRamStatus_t; 


/*! *********************************************************************************
* \brief  This function prepares for sample capture to packet RAM.
*
* \return None.
*
* \details 
* This routine assumes that some other functions in the calling routine both set
* the channel and force RX warmup before calling ::dbg_ram_capture().
***********************************************************************************/
void dbg_ram_init(void);

/*! *********************************************************************************
* \brief  This function captures transceiver data to the transceiver packet RAM.
*
* \param[in] dbg_page - The page selector (DBG_PAGE).
* \param[in] buffer_sz_bytes - The size of the output buffer (in bytes)
* \param[in] result_buffer - The pointer to the output buffer of a size large enough for the samples.
*
* \return None.
*
* \details 
* The capture to packet RAM always captures a full PKT_RAM worth of samples. The samples will be 
* copied to the buffer pointed to by result_buffer parameter until buffer_sz_bytes worth of data have
* been copied. Data will be copied 
* NOTE: This routine has a slight hazard of getting stuck waiting for debug RAM to fill up when RX has
* not been enabled or RX ends before the RAM fills up (such as when capturing packet data ). It is 
* intended to be used with manually triggered RX where RX data will continue as long as needed.
***********************************************************************************/
dbgRamStatus_t dbg_ram_capture(uint8_t dbg_page, uint16_t buffer_sz_bytes, void * result_buffer);


/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _DBG_RAM_CAPTURE_H_ */


