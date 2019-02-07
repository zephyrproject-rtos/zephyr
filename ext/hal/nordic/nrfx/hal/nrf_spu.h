/*
 * Copyright (c) 2018 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_SPU_H__
#define NRF_SPU_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_spu_hal SPU HAL
 * @{
 * @ingroup nrf_spu
 * @brief   Hardware access layer for managing the System Protection Unit (SPU) peripheral.
 */

/** @brief SPU events. */
typedef enum
{
    NRF_SPU_EVENT_RAMACCERR    = offsetof(NRF_SPU_Type, EVENTS_RAMACCERR),   ///< A security violation has been detected for the RAM memory space.
    NRF_SPU_EVENT_FLASHACCERR  = offsetof(NRF_SPU_Type, EVENTS_FLASHACCERR), ///< A security violation has been detected for the Flash memory space.
    NRF_SPU_EVENT_PERIPHACCERR = offsetof(NRF_SPU_Type, EVENTS_PERIPHACCERR) ///< A security violation has been detected on one or several peripherals.
} nrf_spu_event_t;

/** @brief SPU interrupts. */
typedef enum
{
    NRF_SPU_INT_RAMACCERR_MASK     = SPU_INTENSET_RAMACCERR_Msk,   ///< Interrupt on RAMACCERR event.
    NRF_SPU_INT_FLASHACCERR_MASK   = SPU_INTENSET_FLASHACCERR_Msk, ///< Interrupt on FLASHACCERR event.
    NRF_SPU_INT_PERIPHACCERR_MASK  = SPU_INTENSET_PERIPHACCERR_Msk ///< Interrupt on PERIPHACCERR event.
} nrf_spu_int_mask_t;

/** @brief SPU Non-Secure Callable (NSC) region size. */
typedef enum
{
    NRF_SPU_NSC_SIZE_DISABLED = 0, ///< Not defined as a non-secure callable region.
    NRF_SPU_NSC_SIZE_32B      = 1, ///< Non-Secure Callable region with a 32-byte size
    NRF_SPU_NSC_SIZE_64B      = 2, ///< Non-Secure Callable region with a 64-byte size
    NRF_SPU_NSC_SIZE_128B     = 3, ///< Non-Secure Callable region with a 128-byte size
    NRF_SPU_NSC_SIZE_256B     = 4, ///< Non-Secure Callable region with a 256-byte size
    NRF_SPU_NSC_SIZE_512B     = 5, ///< Non-Secure Callable region with a 512-byte size
    NRF_SPU_NSC_SIZE_1024B    = 6, ///< Non-Secure Callable region with a 1024-byte size
    NRF_SPU_NSC_SIZE_2048B    = 7, ///< Non-Secure Callable region with a 2048-byte size
    NRF_SPU_NSC_SIZE_4096B    = 8  ///< Non-Secure Callable region with a 4096-byte size
} nrf_spu_nsc_size_t;

/** @brief SPU memory region permissions. */
typedef enum
{
    NRF_SPU_MEM_PERM_EXECUTE = SPU_FLASHREGION_PERM_EXECUTE_Msk, ///< Allow code execution from particular memory region.
    NRF_SPU_MEM_PERM_WRITE   = SPU_FLASHREGION_PERM_WRITE_Msk,   ///< Allow write operation on particular memory region.
    NRF_SPU_MEM_PERM_READ    = SPU_FLASHREGION_PERM_READ_Msk     ///< Allow read operation from particular memory region.
} nrf_spu_mem_perm_t;

/**
 * @brief Function for clearing a specific SPU event.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] event Event to clear.
 */
__STATIC_INLINE void nrf_spu_event_clear(NRF_SPU_Type *  p_reg,
                                         nrf_spu_event_t event);

/**
 * @brief Function for checking the state of a specific SPU event.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_spu_event_check(NRF_SPU_Type const * p_reg,
                                         nrf_spu_event_t      event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] mask  Interrupts to enable.
 */
__STATIC_INLINE void nrf_spu_int_enable(NRF_SPU_Type * p_reg,
                                        uint32_t       mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] mask  Interrupts to disable.
 */
__STATIC_INLINE void nrf_spu_int_disable(NRF_SPU_Type * p_reg,
                                         uint32_t       mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] spu_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_spu_int_enable_check(NRF_SPU_Type const * p_reg,
                                              uint32_t             spu_int);

/**
 * @brief Function for setting up publication configuration of a given SPU event.
 *
 * @param[in] p_reg   Pointer to the peripheral registers structure.
 * @param[in] event   Event to configure.
 * @param[in] channel Channel to connect with published event.
 */
__STATIC_INLINE void nrf_spu_publish_set(NRF_SPU_Type *  p_reg,
                                         nrf_spu_event_t event,
                                         uint32_t        channel);

/**
 * @brief Function for clearing publication configuration of a given SPU event.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 * @param[in] event Event to clear.
 */
__STATIC_INLINE void nrf_spu_publish_clear(NRF_SPU_Type *  p_reg,
                                           nrf_spu_event_t event);

/**
 * @brief Function for retrieving the capabilities of the current device.
 *
 * @param[in] p_reg Pointer to the peripheral registers structure.
 *
 * @retval true  If ARM TrustZone support is available.
 * @retval false If ARM TrustZone support is not available.
 */
__STATIC_INLINE bool nrf_spu_tz_is_available(NRF_SPU_Type const * p_reg);

/**
 * @brief Function for configuring the DPPI channels to be available in particular domains.
 *
 * Channels are configured as bitmask. Set one in bitmask to make channels available only in secure
 * domain. Set zero to make it available in secure and non-secure domains.
 *
 * @param[in] p_reg         Pointer to the peripheral registers structure.
 * @param[in] dppi_id       DPPI peripheral id.
 * @param[in] channels_mask Bitmask with channels configuration.
 * @param[in] lock_conf     Lock configuration until next SoC reset.
 */
__STATIC_INLINE void nrf_spu_dppi_config_set(NRF_SPU_Type * p_reg,
                                             uint8_t        dppi_id,
                                             uint32_t       channels_mask,
                                             bool           lock_conf);

/**
 * @brief Function for configuring the GPIO pins to be available in particular domains.
 *
 * GPIO pins are configured as bitmask. Set one in bitmask to make particular pin available only
 * in secure domain. Set zero to make it available in secure and non-secure domains.
 *
 * @param[in] p_reg     Pointer to the peripheral registers structure.
 * @param[in] gpio_port Port number.
 * @param[in] gpio_mask Bitmask with gpio configuration.
 * @param[in] lock_conf Lock configuration until next SoC reset.
 */
__STATIC_INLINE void nrf_spu_gpio_config_set(NRF_SPU_Type * p_reg,
                                             uint8_t        gpio_port,
                                             uint32_t       gpio_mask,
                                             bool           lock_conf);

/**
 * @brief Function for configuring non-secure callable flash region.
 *
 * @param[in] p_reg          Pointer to the peripheral registers structure.
 * @param[in] flash_nsc_id   Non-secure callable flash region ID.
 * @param[in] flash_nsc_size Non-secure callable flash region size.
 * @param[in] region_number  Flash region number.
 * @param[in] lock_conf      Lock configuration until next SoC reset.
 */
__STATIC_INLINE void nrf_spu_flashnsc_set(NRF_SPU_Type *     p_reg,
                                          uint8_t            flash_nsc_id,
                                          nrf_spu_nsc_size_t flash_nsc_size,
                                          uint8_t            region_number,
                                          bool               lock_conf);

/**
 * @brief Function for configuring non-secure callable RAM region.
 *
 * @param[in] p_reg         Pointer to the peripheral registers structure.
 * @param[in] ram_nsc_id    Non-secure callable RAM region ID.
 * @param[in] ram_nsc_size  Non-secure callable RAM region size.
 * @param[in] region_number RAM region number.
 * @param[in] lock_conf     Lock configuration until next SoC reset.
 */
__STATIC_INLINE void nrf_spu_ramnsc_set(NRF_SPU_Type *     p_reg,
                                        uint8_t            ram_nsc_id,
                                        nrf_spu_nsc_size_t ram_nsc_size,
                                        uint8_t            region_number,
                                        bool               lock_conf);

/**
 * @brief Function for configuring security for a particular flash region.
 *
 * Permissions parameter must be set by using the logical OR on the @ref nrf_spu_mem_perm_t values.
 *
 * @param[in] p_reg       Pointer to the peripheral registers structure.
 * @param[in] region_id   Flash region index.
 * @param[in] secure_attr Set region attribute to secure.
 * @param[in] permissions Flash region permissions.
 * @param[in] lock_conf   Lock configuration until next SoC reset.
 */
__STATIC_INLINE void nrf_spu_flashregion_set(NRF_SPU_Type * p_reg,
                                             uint8_t        region_id,
                                             bool           secure_attr,
                                             uint32_t       permissions,
                                             bool           lock_conf);

/**
 * @brief Function for configuring security for the RAM region.
 *
 * Permissions parameter must be set by using the logical OR on the @ref nrf_spu_mem_perm_t values.
 *
 * @param[in] p_reg       Pointer to the peripheral registers structure.
 * @param[in] region_id   RAM region index.
 * @param[in] secure_attr Set region attribute to secure.
 * @param[in] permissions RAM region permissions.
 * @param[in] lock_conf   Lock configuration until next SoC reset.
 */
__STATIC_INLINE void nrf_spu_ramregion_set(NRF_SPU_Type * p_reg,
                                           uint8_t        region_id,
                                           bool           secure_attr,
                                           uint32_t       permissions,
                                           bool           lock_conf);

/**
 * @brief Function for configuring access permissions of the peripheral.
 *
 * @param[in] p_reg         Pointer to the peripheral registers structure.
 * @param[in] peripheral_id ID number of a particular peripheral.
 * @param[in] secure_attr   Peripheral registers accessible only from secure domain.
 * @param[in] secure_dma    DMA transfers possible only from RAM memory in secure domain.
 * @param[in] lock_conf     Lock configuration until next SoC reset.
 */
__STATIC_INLINE void nrf_spu_peripheral_set(NRF_SPU_Type * p_reg,
                                            uint32_t       peripheral_id,
                                            bool           secure_attr,
                                            bool           secure_dma,
                                            bool           lock_conf);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_spu_event_clear(NRF_SPU_Type *  p_reg,
                                         nrf_spu_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event));
    (void)dummy;
}

__STATIC_INLINE bool nrf_spu_event_check(NRF_SPU_Type const * p_reg,
                                         nrf_spu_event_t      event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

__STATIC_INLINE void nrf_spu_int_enable(NRF_SPU_Type * p_reg,
                                        uint32_t       mask)
{
    p_reg->INTENSET = mask;
}

__STATIC_INLINE void nrf_spu_int_disable(NRF_SPU_Type * p_reg,
                                         uint32_t       mask)
{
    p_reg->INTENCLR = mask;
}

__STATIC_INLINE bool nrf_spu_int_enable_check(NRF_SPU_Type const * p_reg,
                                              uint32_t             spu_int)
{
    return (bool)(p_reg->INTENSET & spu_int);
}

__STATIC_INLINE void nrf_spu_publish_set(NRF_SPU_Type *  p_reg,
                                         nrf_spu_event_t event,
                                         uint32_t        channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
        (channel | (SPU_PUBLISH_RAMACCERR_EN_Msk));
}

__STATIC_INLINE void nrf_spu_publish_clear(NRF_SPU_Type *  p_reg,
                                           nrf_spu_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}

__STATIC_INLINE bool nrf_spu_tz_is_available(NRF_SPU_Type const * p_reg)
{
    return (p_reg->CAP & SPU_CAP_TZM_Msk ? true : false);
}

__STATIC_INLINE void nrf_spu_dppi_config_set(NRF_SPU_Type * p_reg,
                                             uint8_t        dppi_id,
                                             uint32_t       channels_mask,
                                             bool           lock_conf)
{
    NRFX_ASSERT(!(p_reg->DPPI[dppi_id].LOCK & SPU_DPPI_LOCK_LOCK_Msk));

    p_reg->DPPI[dppi_id].PERM = channels_mask;

    if (lock_conf)
    {
        p_reg->DPPI[dppi_id].LOCK = (SPU_DPPI_LOCK_LOCK_Msk);
    }
}

__STATIC_INLINE void nrf_spu_gpio_config_set(NRF_SPU_Type * p_reg,
                                             uint8_t        gpio_port,
                                             uint32_t       gpio_mask,
                                             bool           lock_conf)
{
    NRFX_ASSERT(!(p_reg->GPIOPORT[gpio_port].LOCK & SPU_GPIOPORT_LOCK_LOCK_Msk));

    p_reg->GPIOPORT[gpio_port].PERM = gpio_mask;

    if (lock_conf)
    {
        p_reg->GPIOPORT[gpio_port].LOCK = (SPU_GPIOPORT_LOCK_LOCK_Msk);
    }
}

__STATIC_INLINE void nrf_spu_flashnsc_set(NRF_SPU_Type *     p_reg,
                                          uint8_t            flash_nsc_id,
                                          nrf_spu_nsc_size_t flash_nsc_size,
                                          uint8_t            region_number,
                                          bool               lock_conf)
{
    NRFX_ASSERT(!(p_reg->FLASHNSC[flash_nsc_id].REGION & SPU_FLASHNSC_REGION_LOCK_Msk));
    NRFX_ASSERT(!(p_reg->FLASHNSC[flash_nsc_id].SIZE & SPU_FLASHNSC_SIZE_LOCK_Msk));

    p_reg->FLASHNSC[flash_nsc_id].REGION = (uint32_t)region_number |
        (lock_conf ? SPU_FLASHNSC_REGION_LOCK_Msk : 0);
    p_reg->FLASHNSC[flash_nsc_id].SIZE = (uint32_t)flash_nsc_size |
        (lock_conf ? SPU_FLASHNSC_SIZE_LOCK_Msk : 0);
}

__STATIC_INLINE void nrf_spu_ramnsc_set(NRF_SPU_Type *     p_reg,
                                        uint8_t            ram_nsc_id,
                                        nrf_spu_nsc_size_t ram_nsc_size,
                                        uint8_t            region_number,
                                        bool               lock_conf)
{
    NRFX_ASSERT(!(p_reg->RAMNSC[ram_nsc_id].REGION & SPU_RAMNSC_REGION_LOCK_Msk));
    NRFX_ASSERT(!(p_reg->RAMNSC[ram_nsc_id].SIZE & SPU_RAMNSC_SIZE_LOCK_Msk));

    p_reg->RAMNSC[ram_nsc_id].REGION = (uint32_t)region_number |
        (lock_conf ? SPU_RAMNSC_REGION_LOCK_Msk : 0);
    p_reg->RAMNSC[ram_nsc_id].SIZE = (uint32_t)ram_nsc_size |
        (lock_conf ? SPU_RAMNSC_SIZE_LOCK_Msk : 0);
}

__STATIC_INLINE void nrf_spu_flashregion_set(NRF_SPU_Type * p_reg,
                                             uint8_t        region_id,
                                             bool           secure_attr,
                                             uint32_t       permissions,
                                             bool           lock_conf)
{
    NRFX_ASSERT(!(p_reg->FLASHREGION[region_id].PERM & SPU_FLASHREGION_PERM_LOCK_Msk));

    p_reg->FLASHREGION[region_id].PERM = permissions         |
        (secure_attr ? SPU_FLASHREGION_PERM_SECATTR_Msk : 0) |
        (lock_conf   ? SPU_FLASHREGION_PERM_LOCK_Msk    : 0);
}

__STATIC_INLINE void nrf_spu_ramregion_set(NRF_SPU_Type * p_reg,
                                           uint8_t        region_id,
                                           bool           secure_attr,
                                           uint32_t       permissions,
                                           bool           lock_conf)
{
    NRFX_ASSERT(!(p_reg->RAMREGION[region_id].PERM & SPU_RAMREGION_PERM_LOCK_Msk));

    p_reg->RAMREGION[region_id].PERM = permissions         |
        (secure_attr ? SPU_RAMREGION_PERM_SECATTR_Msk : 0) |
        (lock_conf   ? SPU_RAMREGION_PERM_LOCK_Msk    : 0);
}

__STATIC_INLINE void nrf_spu_peripheral_set(NRF_SPU_Type * p_reg,
                                            uint32_t       peripheral_id,
                                            bool           secure_attr,
                                            bool           secure_dma,
                                            bool           lock_conf)
{
    NRFX_ASSERT(p_reg->PERIPHID[peripheral_id].PERM & SPU_PERIPHID_PERM_PRESENT_Msk);
    NRFX_ASSERT(!(p_reg->PERIPHID[peripheral_id].PERM & SPU_PERIPHID_PERM_LOCK_Msk));

    p_reg->PERIPHID[peripheral_id].PERM =
         (secure_attr ? SPU_PERIPHID_PERM_SECATTR_Msk : 0) |
         (secure_dma  ? SPU_PERIPHID_PERM_DMASEC_Msk  : 0) |
         (lock_conf   ? SPU_PERIPHID_PERM_LOCK_Msk    : 0);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_SPU_H__
