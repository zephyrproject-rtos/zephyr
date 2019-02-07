/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRF_MWU_H__
#define NRF_MWU_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_mwu_hal MWU HAL
 * @{
 * @ingroup nrf_mwu
 * @brief   Hardware access layer for managing the Memory Watch Unit (MWU) peripheral.
 */

/** @brief MWU events. */
typedef enum
{
    NRF_MWU_EVENT_REGION0_WRITE  = offsetof(NRF_MWU_Type, EVENTS_REGION[0].WA),  ///< Write access to region 0 detected.
    NRF_MWU_EVENT_REGION0_READ   = offsetof(NRF_MWU_Type, EVENTS_REGION[0].RA),  ///< Read access to region 0 detected.
    NRF_MWU_EVENT_REGION1_WRITE  = offsetof(NRF_MWU_Type, EVENTS_REGION[1].WA),  ///< Write access to region 1 detected.
    NRF_MWU_EVENT_REGION1_READ   = offsetof(NRF_MWU_Type, EVENTS_REGION[1].RA),  ///< Read access to region 1 detected.
    NRF_MWU_EVENT_REGION2_WRITE  = offsetof(NRF_MWU_Type, EVENTS_REGION[2].WA),  ///< Write access to region 2 detected.
    NRF_MWU_EVENT_REGION2_READ   = offsetof(NRF_MWU_Type, EVENTS_REGION[2].RA),  ///< Read access to region 2 detected.
    NRF_MWU_EVENT_REGION3_WRITE  = offsetof(NRF_MWU_Type, EVENTS_REGION[3].WA),  ///< Write access to region 3 detected.
    NRF_MWU_EVENT_REGION3_READ   = offsetof(NRF_MWU_Type, EVENTS_REGION[3].RA),  ///< Read access to region 3 detected.
    NRF_MWU_EVENT_PREGION0_WRITE = offsetof(NRF_MWU_Type, EVENTS_PREGION[0].WA), ///< Write access to peripheral region 0 detected.
    NRF_MWU_EVENT_PREGION0_READ  = offsetof(NRF_MWU_Type, EVENTS_PREGION[0].RA), ///< Read access to peripheral region 0 detected.
    NRF_MWU_EVENT_PREGION1_WRITE = offsetof(NRF_MWU_Type, EVENTS_PREGION[1].WA), ///< Write access to peripheral region 1 detected.
    NRF_MWU_EVENT_PREGION1_READ  = offsetof(NRF_MWU_Type, EVENTS_PREGION[1].RA), ///< Read access to peripheral region 1 detected.
} nrf_mwu_event_t;

/** @brief MWU interrupt masks. */
typedef enum
{
    NRF_MWU_INT_REGION0_WRITE_MASK  = MWU_INTEN_REGION0WA_Msk,  ///< Interrupt on REGION[0].WA event.
    NRF_MWU_INT_REGION0_READ_MASK   = MWU_INTEN_REGION0RA_Msk,  ///< Interrupt on REGION[0].RA event.
    NRF_MWU_INT_REGION1_WRITE_MASK  = MWU_INTEN_REGION1WA_Msk,  ///< Interrupt on REGION[1].WA event.
    NRF_MWU_INT_REGION1_READ_MASK   = MWU_INTEN_REGION1RA_Msk,  ///< Interrupt on REGION[1].RA event.
    NRF_MWU_INT_REGION2_WRITE_MASK  = MWU_INTEN_REGION2WA_Msk,  ///< Interrupt on REGION[2].WA event.
    NRF_MWU_INT_REGION2_READ_MASK   = MWU_INTEN_REGION2RA_Msk,  ///< Interrupt on REGION[2].RA event.
    NRF_MWU_INT_REGION3_WRITE_MASK  = MWU_INTEN_REGION3WA_Msk,  ///< Interrupt on REGION[3].WA event.
    NRF_MWU_INT_REGION3_READ_MASK   = MWU_INTEN_REGION3RA_Msk,  ///< Interrupt on REGION[3].RA event.
    NRF_MWU_INT_PREGION0_WRITE_MASK = MWU_INTEN_PREGION0WA_Msk, ///< Interrupt on PREGION[0].WA event.
    NRF_MWU_INT_PREGION0_READ_MASK  = MWU_INTEN_PREGION0RA_Msk, ///< Interrupt on PREGION[0].RA event.
    NRF_MWU_INT_PREGION1_WRITE_MASK = MWU_INTEN_PREGION1WA_Msk, ///< Interrupt on PREGION[1].WA event.
    NRF_MWU_INT_PREGION1_READ_MASK  = MWU_INTEN_PREGION1RA_Msk, ///< Interrupt on PREGION[1].RA event.
} nrf_mwu_int_mask_t;

/** @brief MWU region watch masks. */
typedef enum
{
    NRF_MWU_WATCH_REGION0_WRITE  = MWU_REGIONEN_RGN0WA_Msk,  ///< Region 0 write access watch mask
    NRF_MWU_WATCH_REGION0_READ   = MWU_REGIONEN_RGN0RA_Msk,  ///< Region 0 read access watch mask
    NRF_MWU_WATCH_REGION1_WRITE  = MWU_REGIONEN_RGN1WA_Msk,  ///< Region 1 write access watch mask
    NRF_MWU_WATCH_REGION1_READ   = MWU_REGIONEN_RGN1RA_Msk,  ///< Region 1 read access watch mask
    NRF_MWU_WATCH_REGION2_WRITE  = MWU_REGIONEN_RGN2WA_Msk,  ///< Region 2 write access watch mask
    NRF_MWU_WATCH_REGION2_READ   = MWU_REGIONEN_RGN2RA_Msk,  ///< Region 2 read access watch mask
    NRF_MWU_WATCH_REGION3_WRITE  = MWU_REGIONEN_RGN3WA_Msk,  ///< Region 3 write access watch mask
    NRF_MWU_WATCH_REGION3_READ   = MWU_REGIONEN_RGN3RA_Msk,  ///< Region 3 read access watch mask
    NRF_MWU_WATCH_PREGION0_WRITE = MWU_REGIONEN_PRGN0WA_Msk, ///< Peripheral region 0 write access watch mask
    NRF_MWU_WATCH_PREGION0_READ  = MWU_REGIONEN_PRGN0RA_Msk, ///< Peripheral region 0 read access watch mask
    NRF_MWU_WATCH_PREGION1_WRITE = MWU_REGIONEN_PRGN1WA_Msk, ///< Peripheral region 1 write access watch mask
    NRF_MWU_WATCH_PREGION1_READ  = MWU_REGIONEN_PRGN1RA_Msk, ///< Peripheral region 1 read access watch mask
} nrf_mwu_region_watch_t;

/**
 * @brief Function for checking the state of a specific MWU event.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] mwu_event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_mwu_event_check(NRF_MWU_Type const * p_reg,
                                         nrf_mwu_event_t      mwu_event);

/**
 * @brief Function for clearing a specific MWU event.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] mwu_event Event to clear.
 */
__STATIC_INLINE void nrf_mwu_event_clear(NRF_MWU_Type *  p_reg,
                                         nrf_mwu_event_t mwu_event);

/**
 * @brief Function for getting the address of a specific MWU event register.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] mwu_event Requested event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t nrf_mwu_event_address_get(NRF_MWU_Type const * p_reg,
                                                   nrf_mwu_event_t      mwu_event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] int_mask Interrupts to enable.
 */
__STATIC_INLINE void nrf_mwu_int_enable(NRF_MWU_Type * p_reg, uint32_t int_mask);

/**
 * @brief Function for retrieving the state of a specific interrupt.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] mwu_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_mwu_int_enable_check(NRF_MWU_Type const * p_reg,
                                              nrf_mwu_int_mask_t   mwu_int);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] int_mask Interrupts to disable.
 */
__STATIC_INLINE void nrf_mwu_int_disable(NRF_MWU_Type * p_reg, uint32_t int_mask);

/**
 * @brief Function for enabling specified non-maskable interrupts.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] int_mask Interrupts to enable.
 */
__STATIC_INLINE void nrf_mwu_nmi_enable(NRF_MWU_Type * p_reg, uint32_t int_mask);

/**
 * @brief Function for retrieving the state of a specific non-maskable interrupt.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] mwu_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_mwu_nmi_enable_check(NRF_MWU_Type const * p_reg,
                                              nrf_mwu_int_mask_t   mwu_int);

/**
 * @brief Function for disabling specified non-maskable interrupts.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] int_mask Interrupts to disable.
 */
__STATIC_INLINE void nrf_mwu_nmi_disable(NRF_MWU_Type * p_reg, uint32_t int_mask);

/**
 * @brief Function for setting address range of the specified user region.
 *
 * @param[in] p_reg      Pointer to the structure of registers of the peripheral.
 * @param[in] region_idx Region number to configure.
 * @param[in] start_addr Memory address defining the beginning of the region.
 * @param[in] end_addr   Memory address defining the end of the region.
 */
__STATIC_INLINE void nrf_mwu_user_region_range_set(NRF_MWU_Type * p_reg,
                                                   uint8_t        region_idx,
                                                   uint32_t       start_addr,
                                                   uint32_t       end_addr);

/**
 * @brief Function for enabling memory access watch mechanism.
 *
 * @param[in] p_reg          Pointer to the structure of registers of the peripheral.
 * @param[in] reg_watch_mask Mask that defines regions and access types to watch.
 *                           Compose this mask from @ref nrf_mwu_region_watch_t values.
 */
__STATIC_INLINE void nrf_mwu_region_watch_enable(NRF_MWU_Type * p_reg, uint32_t reg_watch_mask);

/**
 * @brief Function for disabling memory access watch mechanism.
 *
 * @param[in] p_reg          Pointer to the structure of registers of the peripheral.
 * @param[in] reg_watch_mask Mask that defines regions and access types to stop watching.
 *                           Compose this mask from @ref nrf_mwu_region_watch_t values.
 */
__STATIC_INLINE void nrf_mwu_region_watch_disable(NRF_MWU_Type * p_reg, uint32_t reg_watch_mask);

/**
 * @brief Function for getting memory access watch configuration mask.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Mask that defines regions and access types being watched.
 */
__STATIC_INLINE uint32_t nrf_mwu_region_watch_get(NRF_MWU_Type const * p_reg);

/**
 * @brief Function for configuring peripheral subregions for watching.
 *
 * @param[in] p_reg          Pointer to the structure of registers of the peripheral.
 * @param[in] per_reg_idx    Peripheral region containing specified subregions.
 * @param[in] subregion_mask Mask that defines subregions to include into the specified peripheral region.
 */
__STATIC_INLINE void nrf_mwu_subregions_configure(NRF_MWU_Type * p_reg,
                                                  uint8_t        per_reg_idx,
                                                  uint32_t       subregion_mask);

/**
 * @brief Function for getting the mask of the write access flags of peripheral subregions
 *
 * @param[in] p_reg       Pointer to the structure of registers of the peripheral.
 * @param[in] per_reg_idx Peripheral region containing subregions to check.
 *
 * @return Mask specifying subregions that were write accessed.
 */
__STATIC_INLINE uint32_t nrf_mwu_subregions_write_accesses_get(NRF_MWU_Type const * p_reg,
                                                               uint8_t              per_reg_idx);

/**
 * @brief Function for clearing write access flags of peripheral subregions.
 *
 * @param[in] p_reg          Pointer to the structure of registers of the peripheral.
 * @param[in] per_reg_idx    Peripheral region containing subregion accesses to clear.
 * @param[in] subregion_mask Mask that defines subregion write accesses to clear.
 */
__STATIC_INLINE void nrf_mwu_subregions_write_accesses_clear(NRF_MWU_Type * p_reg,
                                                             uint8_t        per_reg_idx,
                                                             uint32_t       subregion_mask);

/**
 * @brief Function for getting the mask of the read access flags of peripheral subregions
 *
 * @param[in] p_reg       Pointer to the structure of registers of the peripheral.
 * @param[in] per_reg_idx Peripheral region containing subregions to check.
 *
 * @return Mask specifying subregions that were read accessed.
 */
__STATIC_INLINE uint32_t nrf_mwu_subregions_read_accesses_get(NRF_MWU_Type const * p_reg,
                                                              uint8_t              per_reg_idx);

/**
 * @brief Function for clearing read access flags of peripheral subregions.
 *
 * @param[in] p_reg          Pointer to the structure of registers of the peripheral.
 * @param[in] per_reg_idx    Peripheral region containing subregion accesses to clear.
 * @param[in] subregion_mask Mask that defines subregion read accesses to clear.
 */
__STATIC_INLINE void nrf_mwu_subregions_read_accesses_clear(NRF_MWU_Type * p_reg,
                                                            uint8_t        per_reg_idx,
                                                            uint32_t       subregion_mask);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE bool nrf_mwu_event_check(NRF_MWU_Type const * p_reg,
                                         nrf_mwu_event_t      mwu_event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)mwu_event);
}

__STATIC_INLINE void nrf_mwu_event_clear(NRF_MWU_Type *  p_reg,
                                         nrf_mwu_event_t mwu_event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)mwu_event)) = 0;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)mwu_event));
    (void)dummy;
#endif
}

__STATIC_INLINE uint32_t nrf_mwu_event_address_get(NRF_MWU_Type const * p_reg,
                                                   nrf_mwu_event_t      mwu_event)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)mwu_event);
}

__STATIC_INLINE void nrf_mwu_int_enable(NRF_MWU_Type * p_reg, uint32_t int_mask)
{
    p_reg->INTENSET = int_mask;
}

__STATIC_INLINE bool nrf_mwu_int_enable_check(NRF_MWU_Type const * p_reg,
                                              nrf_mwu_int_mask_t   mwu_int)
{
    return (bool)(p_reg->INTENSET & mwu_int);
}

__STATIC_INLINE void nrf_mwu_int_disable(NRF_MWU_Type * p_reg, uint32_t int_mask)
{
    p_reg->INTENCLR = int_mask;
}

__STATIC_INLINE void nrf_mwu_nmi_enable(NRF_MWU_Type * p_reg, uint32_t int_mask)
{
    p_reg->NMIENSET = int_mask;
}

__STATIC_INLINE bool nrf_mwu_nmi_enable_check(NRF_MWU_Type const * p_reg,
                                              nrf_mwu_int_mask_t   mwu_int)
{
    return (bool)(p_reg->NMIENSET & mwu_int);
}

__STATIC_INLINE void nrf_mwu_nmi_disable(NRF_MWU_Type * p_reg, uint32_t int_mask)
{
    p_reg->NMIENCLR = int_mask;
}

__STATIC_INLINE void nrf_mwu_user_region_range_set(NRF_MWU_Type * p_reg,
                                                   uint8_t        region_idx,
                                                   uint32_t       start_addr,
                                                   uint32_t       end_addr)
{
    NRFX_ASSERT(region_idx < NRFX_ARRAY_SIZE(NRF_MWU->REGION));
    NRFX_ASSERT(end_addr >= start_addr);

    p_reg->REGION[region_idx].START = start_addr;
    p_reg->REGION[region_idx].END = end_addr;
}

__STATIC_INLINE void nrf_mwu_region_watch_enable(NRF_MWU_Type * p_reg, uint32_t reg_watch_mask)
{
    p_reg->REGIONENSET = reg_watch_mask;
}

__STATIC_INLINE void nrf_mwu_region_watch_disable(NRF_MWU_Type * p_reg, uint32_t reg_watch_mask)
{
    p_reg->REGIONENCLR = reg_watch_mask;
}

__STATIC_INLINE uint32_t nrf_mwu_region_watch_get(NRF_MWU_Type const * p_reg)
{
    return p_reg->REGIONENSET;
}

__STATIC_INLINE void nrf_mwu_subregions_configure(NRF_MWU_Type * p_reg,
                                                  uint8_t        per_reg_idx,
                                                  uint32_t       subregion_mask)
{
    NRFX_ASSERT(per_reg_idx < NRFX_ARRAY_SIZE(NRF_MWU->PREGION));

    p_reg->PREGION[per_reg_idx].SUBS = subregion_mask;
}

__STATIC_INLINE uint32_t nrf_mwu_subregions_write_accesses_get(NRF_MWU_Type const * p_reg,
                                                               uint8_t              per_reg_idx)
{
    NRFX_ASSERT(per_reg_idx < NRFX_ARRAY_SIZE(NRF_MWU->PREGION));

    return p_reg->PERREGION[per_reg_idx].SUBSTATWA;
}

__STATIC_INLINE void nrf_mwu_subregions_write_accesses_clear(NRF_MWU_Type * p_reg,
                                                             uint8_t        per_reg_idx,
                                                             uint32_t       subregion_mask)
{
    NRFX_ASSERT(per_reg_idx < NRFX_ARRAY_SIZE(NRF_MWU->PREGION));

    p_reg->PERREGION[per_reg_idx].SUBSTATWA = subregion_mask;
}

__STATIC_INLINE uint32_t nrf_mwu_subregions_read_accesses_get(NRF_MWU_Type const * p_reg,
                                                              uint8_t              per_reg_idx)
{
    NRFX_ASSERT(per_reg_idx < NRFX_ARRAY_SIZE(NRF_MWU->PREGION));

    return p_reg->PERREGION[per_reg_idx].SUBSTATRA;
}

__STATIC_INLINE void nrf_mwu_subregions_read_accesses_clear(NRF_MWU_Type * p_reg,
                                                            uint8_t        per_reg_idx,
                                                            uint32_t       subregion_mask)
{
    NRFX_ASSERT(per_reg_idx < NRFX_ARRAY_SIZE(NRF_MWU->PREGION));

    p_reg->PERREGION[per_reg_idx].SUBSTATRA = subregion_mask;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_MWU_H__
