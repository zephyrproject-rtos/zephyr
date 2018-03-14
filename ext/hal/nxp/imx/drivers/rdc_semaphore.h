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

#ifndef __RDC_SEMAPHORE_H__
#define __RDC_SEMAPHORE_H__

#include <stdint.h>
#include <stdbool.h>
#include "device_imx.h"

/*!
 * @addtogroup rdc_semaphore_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define RDC_SEMAPHORE_MASTER_NONE (0xFF)

/*! @brief RDC Semaphore status return codes. */
typedef enum _rdc_semaphore_status
{
    statusRdcSemaphoreSuccess = 0U, /*!< Success.                                          */
    statusRdcSemaphoreBusy    = 1U, /*!< RDC semaphore has been locked by other processor. */
} rdc_semaphore_status_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name RDC_SEMAPHORE State Control
 * @{
 */

/*!
 * @brief Lock RDC semaphore for shared peripheral access
 *
 * @param pdap RDC peripheral assignment (see @ref _rdc_pdap in rdc_defs_<device>.h)
 * @retval statusRdcSemaphoreSuccess Lock the semaphore successfully.
 * @retval statusRdcSemaphoreBusy    Semaphore has been locked by other processor.
 */
rdc_semaphore_status_t RDC_SEMAPHORE_TryLock(uint32_t pdap);

/*!
 * @brief Lock RDC semaphore for shared peripheral access, polling until success.
 *
 * @param pdap RDC peripheral assignment (see @ref _rdc_pdap in rdc_defs_<device>.h)
 */
void RDC_SEMAPHORE_Lock(uint32_t pdap);

/*!
 * @brief Unlock RDC semaphore
 *
 * @param pdap RDC peripheral assignment (see @ref _rdc_pdap in rdc_defs_<device>.h)
 */
void RDC_SEMAPHORE_Unlock(uint32_t pdap);

/*!
 * @brief Get domain ID which locks the semaphore
 *
 * @param pdap RDC peripheral assignment (see @ref _rdc_pdap in rdc_defs_<device>.h)
 * @return domain ID which locks the RDC semaphore
 */
uint32_t RDC_SEMAPHORE_GetLockDomainID(uint32_t pdap);

/*!
 * @brief Get master index which locks the semaphore
 *
 * @param pdap RDC peripheral assignment (see @ref _rdc_pdap in rdc_defs_<device>.h)
 * @return master index which locks the RDC semaphore, or RDC_SEMAPHORE_MASTER_NONE
 *         to indicate it is not locked.
 */
uint32_t RDC_SEMAPHORE_GetLockMaster(uint32_t pdap);

/*@}*/

/*!
 * @name RDC_SEMAPHORE Reset Control
 * @{
 */

/*!
 * @brief Reset RDC semaphore to unlocked status
 *
 * @param pdap RDC peripheral assignment (see @ref _rdc_pdap in rdc_defs_<device>.h)
 */
void RDC_SEMAPHORE_Reset(uint32_t pdap);

/*!
 * @brief Reset all RDC semaphore to unlocked status for certain RDC_SEMAPHORE instance
 *
 * @param base RDC semaphore base pointer.
 */
void RDC_SEMAPHORE_ResetAll(RDC_SEMAPHORE_Type *base);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __RDC_SEMAPHORE_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
