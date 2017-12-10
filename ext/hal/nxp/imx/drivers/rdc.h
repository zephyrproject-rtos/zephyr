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

#ifndef __RDC_H__
#define __RDC_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup rdc_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name RDC State Control
 * @{
 */

/*!
 * @brief Get domain ID of core that is reading this
 *
 * @param base RDC base pointer.
 * @return Domain ID of self core
 */
static inline uint32_t RDC_GetSelfDomainID(RDC_Type * base)
{
    return (base->STAT & RDC_STAT_DID_MASK) >> RDC_STAT_DID_SHIFT;
}

/*!
 * @brief Check whether memory region controlled by RDC is accessible after low power recovery
 *
 * @param base RDC base pointer.
 * @return Memory region power status.
 *         - true: on and accessible.
 *         - false: off.
 */
static inline bool RDC_IsMemPowered(RDC_Type * base)
{
    return (bool)(base->STAT & RDC_STAT_PDS_MASK);
}

/*!
 * @brief Check whether there's pending RDC memory region restoration interrupt
 *
 * @param base RDC base pointer.
 * @return RDC interrupt status
 *         - true: Interrupt pending.
 *         - false: No interrupt pending.
 */
static inline bool RDC_IsIntPending(RDC_Type * base)
{
    return (bool)(base->INTSTAT);
}

/*!
 * @brief Clear interrupt status
 *
 * @param base RDC base pointer.
 */
static inline void RDC_ClearStatusFlag(RDC_Type * base)
{
    base->INTSTAT = RDC_INTSTAT_INT_MASK;
}

/*!
 * @brief Set RDC interrupt mode
 *
 * @param base RDC base pointer
 * @param enable RDC interrupt control.
 *               - true: enable interrupt.
 *               - false: disable interrupt.
 */
static inline void RDC_SetIntCmd(RDC_Type * base, bool enable)
{
    base->INTCTRL = enable ? RDC_INTCTRL_RCI_EN_MASK : 0;
}

/*@}*/

/*!
 * @name RDC Domain Control
 * @{
 */

/*!
 * @brief Set RDC domain ID for RDC master
 *
 * @param base RDC base pointer
 * @param mda RDC master assignment (see @ref _rdc_mda in rdc_defs_<device>.h)
 * @param domainId RDC domain ID (0-3)
 * @param lock Whether to lock this setting? Once locked, no one can change the domain assignment until reset
 */
static inline void RDC_SetDomainID(RDC_Type * base, uint32_t mda, uint32_t domainId, bool lock)
{
    assert (domainId <= RDC_MDA_DID_MASK);

    base->MDA[mda] = RDC_MDA_DID(domainId) | (lock ? RDC_MDA_LCK_MASK : 0);
}

/*!
 * @brief Get RDC domain ID for RDC master
 *
 * @param base RDC base pointer
 * @param mda RDC master assignment (see @ref _rdc_mda in rdc_defs_<device>.h)
 * @return RDC domain ID (0-3)
 */
static inline uint32_t RDC_GetDomainID(RDC_Type * base, uint32_t mda)
{
    return base->MDA[mda] & RDC_MDA_DID_MASK;
}

/*!
 * @brief Set RDC peripheral access permission for RDC domains
 *
 * @param base RDC base pointer
 * @param pdap RDC peripheral assignment (see @ref _rdc_pdap in rdc_defs_<device>.h)
 * @param perm RDC access permission from RDC domain to peripheral (byte: D3R D3W D2R D2W D1R D1W D0R D0W)
 * @param sreq Force acquiring SEMA42 to access this peripheral or not
 * @param lock Whether to lock this setting or not. Once locked, no one can change the RDC setting until reset
 */
static inline void RDC_SetPdapAccess(RDC_Type * base, uint32_t pdap, uint8_t perm, bool sreq, bool lock)
{
    base->PDAP[pdap] = perm | (sreq ? RDC_PDAP_SREQ_MASK : 0) | (lock ? RDC_PDAP_LCK_MASK : 0);
}

/*!
 * @brief Get RDC peripheral access permission for RDC domains
 *
 * @param base RDC base pointer
 * @param pdap RDC peripheral assignment (see @ref _rdc_pdap in rdc_defs_<device>.h)
 * @return RDC access permission from RDC domain to peripheral (byte: D3R D3W D2R D2W D1R D1W D0R D0W)
 */
static inline uint8_t RDC_GetPdapAccess(RDC_Type * base, uint32_t pdap)
{
    return base->PDAP[pdap] & 0xFF;
}

/*!
 * @brief Check whether RDC semaphore is required to access the peripheral
 *
 * @param base RDC base pointer
 * @param pdap RDC peripheral assignment (see @ref _rdc_pdap in rdc_defs_<device>.h)
 * @return RDC semaphore required or not.
 *         - true: RDC semaphore is required.
 *         - false: RDC semaphore is not required.
 */
static inline bool RDC_IsPdapSemaphoreRequired(RDC_Type * base, uint32_t pdap)
{
    return (bool)(base->PDAP[pdap] & RDC_PDAP_SREQ_MASK);
}

/*!
 * @brief Set RDC memory region access permission for RDC domains
 *
 * @param base RDC base pointer
 * @param mr RDC memory region assignment (see @ref _rdc_mr in rdc_defs_<device>.h)
 * @param startAddr memory region start address (inclusive)
 * @param endAddr memory region end address (exclusive)
 * @param perm RDC access permission from RDC domain to peripheral (byte: D3R D3W D2R D2W D1R D1W D0R D0W)
 * @param enable Enable this memory region for RDC control or not
 * @param lock Whether to lock this setting or not. Once locked, no one can change the RDC setting until reset
 */
void RDC_SetMrAccess(RDC_Type * base, uint32_t mr, uint32_t startAddr, uint32_t endAddr,
                     uint8_t perm, bool enable, bool lock);

/*!
 * @brief Get RDC memory region access permission for RDC domains
 *
 * @param base RDC base pointer
 * @param mr RDC memory region assignment (see @ref _rdc_mr in rdc_defs_<device>.h)
 * @param startAddr pointer to get memory region start address (inclusive), NULL is allowed.
 * @param endAddr pointer to get memory region end address (exclusive), NULL is allowed.
 * @return RDC access permission from RDC domain to peripheral (byte: D3R D3W D2R D2W D1R D1W D0R D0W)
 */
uint8_t RDC_GetMrAccess(RDC_Type * base, uint32_t mr, uint32_t *startAddr, uint32_t *endAddr);


/*!
 * @brief Check whether the memory region is enabled
 *
 * @param base RDC base pointer
 * @param mr RDC memory region assignment (see _rdc_mr in rdc_defs_<device>.h)
 * @return Memory region enabled or not.
 *         - true: Memory region is enabled.
 *         - false: Memory region is not enabled.
 */
static inline bool RDC_IsMrEnabled(RDC_Type * base, uint32_t mr)
{
    return (bool)(base->MR[mr].MRC & RDC_MRC_ENA_MASK);
}

/*!
 * @brief Get memory violation status
 *
 * @param base RDC base pointer
 * @param mr RDC memory region assignment (see @ref _rdc_mr in rdc_defs_<device>.h)
 * @param violationAddr Pointer to store violation address, NULL allowed
 * @param violationDomain Pointer to store domain ID causing violation, NULL allowed
 * @return Memory violation occurred or not.
 *         - true: violation happened.
 *         - false: No violation happened.
 */
bool RDC_GetViolationStatus(RDC_Type * base, uint32_t mr, uint32_t *violationAddr, uint32_t *violationDomain);

/*!
 * @brief Clear RDC violation status
 *
 * @param base RDC base pointer
 * @param mr RDC memory region assignment (see @ref _rdc_mr in rdc_defs_<device>.h)
 */
static inline void RDC_ClearViolationStatus(RDC_Type * base, uint32_t mr)
{
    base->MR[mr].MRVS = RDC_MRVS_AD_MASK;
}

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __RDC_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
