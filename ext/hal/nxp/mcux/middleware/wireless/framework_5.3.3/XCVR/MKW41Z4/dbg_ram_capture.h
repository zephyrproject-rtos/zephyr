/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
    DBG_RAM_INVALID_TRIG_SETTING = 4,
    DBG_RAM_FAIL_NOT_ENOUGH_SAMPLES = 5,
    DBG_RAM_CAPTURE_NOT_COMPLETE = 6, /* Not an error response, but an indication that capture isn't complete for status polling */
} dbgRamStatus_t;

#if RADIO_IS_GEN_3P0
typedef enum _dbgRamStartTriggerType
{
    NO_START_TRIG = 0,
    START_ON_FSK_PREAMBLE_FOUND = 1,
    START_ON_FSK_AA_MATCH = 2,
    START_ON_ZBDEMOD_PREAMBLE_FOUND = 3,
    START_ON_ZBDEMOD_SFD_MATCH = 4,
    START_ON_AGC_DCOC_GAIN_CHG = 5,
    START_ON_TSM_RX_DIG_EN = 6,
    START_ON_TSM_SPARE2_EN = 7,
    INVALID_START_TRIG = 8
} dbgRamStartTriggerType;

typedef enum _dbgRamStopTriggerType
{
    NO_STOP_TRIG = 0,
    STOP_ON_FSK_PREAMBLE_FOUND = 1,
    STOP_ON_FSK_AA_MATCH = 2,
    STOP_ON_ZBDEMOD_PREAMBLE_FOUND = 3,
    STOP_ON_ZBDEMOD_SFD_MATCH = 4,
    STOP_ON_AGC_DCOC_GAIN_CHG = 5,
    STOP_ON_TSM_RX_DIG_EN = 6,
    STOP_ON_TSM_SPARE3_EN = 7,
    STOP_ON_TSM_PLL_UNLOCK = 8,
    STOP_ON_BLE_CRC_ERROR_INC = 9,
    STOP_ON_CRC_FAIL_ZGBE_GENFSK = 10,
    STOP_ON_GENFSK_HEADER_FAIL = 11,
    INVALID_STOP_TRIG = 12
} dbgRamStopTriggerType;
#endif /* RADIO_IS_GEN_3P0 */

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
 * \brief  This function performs any state restoration at the completion of PKT RAM capture.
 * 
 * \details
 * Any clocks enabled to the packet RAM capture circuitry are disabled.
 ***********************************************************************************/
void dbg_ram_release(void);

#if RADIO_IS_GEN_3P0
/*! *********************************************************************************
 * \brief  This function initiates the capture of transceiver data to the transceiver packet RAM.
 *
 * \param[in] dbg_page - The page selector (DBG_PAGE).
 * \param[in] dbg_start_trigger - The trigger to start acquisition (must be "no trigger" if a stop trigger is enabled).
 * \param[in] dbg_stop_trigger - The trigger to stop acquisition (must be "no trigger" if a start trigger is enabled).
 *
 * \return Status of the request.
 *
 * \details 
 * This function starts the process of capturing data to the packet RAM. Depending upon the start and stop trigger
 * settings, the actual capture process can take an indeterminate amount of time. Other APIs are provided to 
 * perform a blocking wait for completion or allow polling for completion of the capture.
 * After any capture has completed, a separate routine must be called to postprocess the capture and copy all
 * data out of the packet RAM into a normal RAM buffer.
 ***********************************************************************************/
dbgRamStatus_t dbg_ram_start_capture(uint8_t dbg_page, dbgRamStartTriggerType start_trig, dbgRamStopTriggerType stop_trig);

/*! *********************************************************************************
 * \brief  This function performs a blocking wait for completion of the capture of transceiver data to the transceiver packet RAM.
 *
 * \return Status of the request, DBG_RAM_SUCCESS if capture is complete.
 *
 * \details 
 * This function performs a wait loop for capture completion and may take an indeterminate amount of time for 
 * some capture trigger types.
 ***********************************************************************************/
dbgRamStatus_t dbg_ram_wait_for_complete(void); /* Blocking wait for capture completion, no matter what trigger type */

/*! *********************************************************************************
 * \brief  This function polls the state of the capture of transceiver data to the transceiver packet RAM.
 *
 * \return Status of the request, DBG_RAM_SUCCESS if capture is complete, DBG_RAM_CAPTURE_NOT_COMPLETE if not complete.
 *
 ***********************************************************************************/
dbgRamStatus_t dbg_ram_poll_capture_status(void); /* Non-blocking completion check, just reads the current status of the capure */

/*! *********************************************************************************
 * \brief  This function processes the captured data into a usable order and copies from packet RAM to normal RAM.
 *
 * \param[in] dbg_page - The page selector (DBG_PAGE).
 * \param[in] buffer_sz_bytes - The size of the output buffer (in bytes)
 * \param[in] result_buffer - The pointer to the output buffer of a size large enough for the samples.
 *
 * \return None.
 *
 * \details 
 * Data is copied from packet RAM in bytes to ensure no access problems. Data is unpacked from packet RAM 
 * (either sequentially captured or simultaneously captured) into a linear RAM buffer in system RAM. 
 * If a start trigger is enabled then the first buffer_sz_bytes that are captured are copied out.
 * If a stop trigger is enabled then the last buffer_sz_bytes that are captured are copied out.
 ***********************************************************************************/
dbgRamStatus_t dbg_ram_postproc_capture(uint8_t dbg_page, uint16_t buffer_sz_bytes, void * result_buffer); /* postprocess a capture to unpack data */

#else
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
#endif /* RADIO_IS_GEN_3P0 */

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _DBG_RAM_CAPTURE_H_ */

