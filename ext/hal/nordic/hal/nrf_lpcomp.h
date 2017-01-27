/* Copyright (c) 2014-2017 Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
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
 */

/**
 * @file
 * @brief LPCOMP HAL API.
 */

#ifndef NRF_LPCOMP_H_
#define NRF_LPCOMP_H_

/**
 * @defgroup nrf_lpcomp_hal LPCOMP HAL
 * @{
 * @ingroup nrf_lpcomp
 * @brief Hardware access layer for managing the Low Power Comparator (LPCOMP).
 */

#include "nrf.h"
#include "nrf_peripherals.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum nrf_lpcomp_ref_t
 * @brief LPCOMP reference selection.
 */
typedef enum
{
#if (LPCOMP_REFSEL_RESOLUTION == 8) || defined(__SDK_DOXYGEN__)
    NRF_LPCOMP_REF_SUPPLY_1_8   = LPCOMP_REFSEL_REFSEL_SupplyOneEighthPrescaling,    /**< Use supply with a 1/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_2_8   = LPCOMP_REFSEL_REFSEL_SupplyTwoEighthsPrescaling,   /**< Use supply with a 2/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_3_8   = LPCOMP_REFSEL_REFSEL_SupplyThreeEighthsPrescaling, /**< Use supply with a 3/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_4_8   = LPCOMP_REFSEL_REFSEL_SupplyFourEighthsPrescaling,  /**< Use supply with a 4/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_5_8   = LPCOMP_REFSEL_REFSEL_SupplyFiveEighthsPrescaling,  /**< Use supply with a 5/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_6_8   = LPCOMP_REFSEL_REFSEL_SupplySixEighthsPrescaling,   /**< Use supply with a 6/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_7_8   = LPCOMP_REFSEL_REFSEL_SupplySevenEighthsPrescaling, /**< Use supply with a 7/8 prescaler as reference. */
#elif (LPCOMP_REFSEL_RESOLUTION == 16) || defined(__SDK_DOXYGEN__)
    NRF_LPCOMP_REF_SUPPLY_1_8   = LPCOMP_REFSEL_REFSEL_Ref1_8Vdd, /**< Use supply with a 1/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_2_8   = LPCOMP_REFSEL_REFSEL_Ref2_8Vdd, /**< Use supply with a 2/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_3_8   = LPCOMP_REFSEL_REFSEL_Ref3_8Vdd, /**< Use supply with a 3/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_4_8   = LPCOMP_REFSEL_REFSEL_Ref4_8Vdd, /**< Use supply with a 4/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_5_8   = LPCOMP_REFSEL_REFSEL_Ref5_8Vdd, /**< Use supply with a 5/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_6_8   = LPCOMP_REFSEL_REFSEL_Ref6_8Vdd, /**< Use supply with a 6/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_7_8   = LPCOMP_REFSEL_REFSEL_Ref7_8Vdd, /**< Use supply with a 7/8 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_1_16  = LPCOMP_REFSEL_REFSEL_Ref1_16Vdd, /**< Use supply with a 1/16 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_3_16  = LPCOMP_REFSEL_REFSEL_Ref3_16Vdd, /**< Use supply with a 3/16 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_5_16  = LPCOMP_REFSEL_REFSEL_Ref5_16Vdd, /**< Use supply with a 5/16 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_7_16  = LPCOMP_REFSEL_REFSEL_Ref7_16Vdd, /**< Use supply with a 7/16 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_9_16  = LPCOMP_REFSEL_REFSEL_Ref9_16Vdd, /**< Use supply with a 9/16 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_11_16 = LPCOMP_REFSEL_REFSEL_Ref11_16Vdd, /**< Use supply with a 11/16 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_13_16 = LPCOMP_REFSEL_REFSEL_Ref13_16Vdd, /**< Use supply with a 13/16 prescaler as reference. */
    NRF_LPCOMP_REF_SUPPLY_15_16 = LPCOMP_REFSEL_REFSEL_Ref15_16Vdd, /**< Use supply with a 15/16 prescaler as reference. */
#endif
    NRF_LPCOMP_REF_EXT_REF0        = LPCOMP_REFSEL_REFSEL_ARef |
                       (LPCOMP_EXTREFSEL_EXTREFSEL_AnalogReference0 << 16), /**< External reference 0. */
    NRF_LPCOMP_CONFIG_REF_EXT_REF1 = LPCOMP_REFSEL_REFSEL_ARef |
                        (LPCOMP_EXTREFSEL_EXTREFSEL_AnalogReference1 << 16), /**< External reference 1. */
} nrf_lpcomp_ref_t;

/**
 * @enum nrf_lpcomp_input_t
 * @brief LPCOMP input selection.
 */
typedef enum
{
    NRF_LPCOMP_INPUT_0 = LPCOMP_PSEL_PSEL_AnalogInput0, /**< Input 0. */
    NRF_LPCOMP_INPUT_1 = LPCOMP_PSEL_PSEL_AnalogInput1, /**< Input 1. */
    NRF_LPCOMP_INPUT_2 = LPCOMP_PSEL_PSEL_AnalogInput2, /**< Input 2. */
    NRF_LPCOMP_INPUT_3 = LPCOMP_PSEL_PSEL_AnalogInput3, /**< Input 3. */
    NRF_LPCOMP_INPUT_4 = LPCOMP_PSEL_PSEL_AnalogInput4, /**< Input 4. */
    NRF_LPCOMP_INPUT_5 = LPCOMP_PSEL_PSEL_AnalogInput5, /**< Input 5. */
    NRF_LPCOMP_INPUT_6 = LPCOMP_PSEL_PSEL_AnalogInput6, /**< Input 6. */
    NRF_LPCOMP_INPUT_7 = LPCOMP_PSEL_PSEL_AnalogInput7  /**< Input 7. */
} nrf_lpcomp_input_t;

/**
 * @enum nrf_lpcomp_detect_t
 * @brief LPCOMP detection type selection.
 */
typedef enum
{
    NRF_LPCOMP_DETECT_CROSS = LPCOMP_ANADETECT_ANADETECT_Cross, /**< Generate ANADETEC on crossing, both upwards and downwards crossing. */
    NRF_LPCOMP_DETECT_UP    = LPCOMP_ANADETECT_ANADETECT_Up,    /**< Generate ANADETEC on upwards crossing only. */
    NRF_LPCOMP_DETECT_DOWN  = LPCOMP_ANADETECT_ANADETECT_Down   /**< Generate ANADETEC on downwards crossing only. */
} nrf_lpcomp_detect_t;

/**
 * @enum nrf_lpcomp_task_t
 * @brief LPCOMP tasks.
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    NRF_LPCOMP_TASK_START  = offsetof(NRF_LPCOMP_Type, TASKS_START), /**< LPCOMP start sampling task. */
    NRF_LPCOMP_TASK_STOP   = offsetof(NRF_LPCOMP_Type, TASKS_STOP),  /**< LPCOMP stop sampling task. */
    NRF_LPCOMP_TASK_SAMPLE = offsetof(NRF_LPCOMP_Type, TASKS_SAMPLE) /**< Sample comparator value. */
} nrf_lpcomp_task_t;                                                 /*lint -restore*/


/**
 * @enum nrf_lpcomp_event_t
 * @brief LPCOMP events.
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    NRF_LPCOMP_EVENT_READY = offsetof(NRF_LPCOMP_Type, EVENTS_READY), /**< LPCOMP is ready and output is valid. */
    NRF_LPCOMP_EVENT_DOWN  = offsetof(NRF_LPCOMP_Type, EVENTS_DOWN),  /**< Input voltage crossed the threshold going down. */
    NRF_LPCOMP_EVENT_UP    = offsetof(NRF_LPCOMP_Type, EVENTS_UP),    /**< Input voltage crossed the threshold going up. */
    NRF_LPCOMP_EVENT_CROSS = offsetof(NRF_LPCOMP_Type, EVENTS_CROSS)  /**< Input voltage crossed the threshold in any direction. */
} nrf_lpcomp_event_t;                                                 /*lint -restore*/

/**
 * @enum nrf_lpcomp_short_mask_t
 * @brief LPCOMP shorts masks.
 */
typedef enum
{
    NRF_LPCOMP_SHORT_CROSS_STOP_MASK   = LPCOMP_SHORTS_CROSS_STOP_Msk,  /*!< Short between CROSS event and STOP task. */
    NRF_LPCOMP_SHORT_UP_STOP_MASK      = LPCOMP_SHORTS_UP_STOP_Msk,     /*!< Short between UP event and STOP task. */
    NRF_LPCOMP_SHORT_DOWN_STOP_MASK    = LPCOMP_SHORTS_DOWN_STOP_Msk,   /*!< Short between DOWN event and STOP task. */
    NRF_LPCOMP_SHORT_READY_STOP_MASK   = LPCOMP_SHORTS_READY_STOP_Msk,  /*!< Short between READY event and STOP task. */
    NRF_LPCOMP_SHORT_READY_SAMPLE_MASK = LPCOMP_SHORTS_READY_SAMPLE_Msk /*!< Short between READY event and SAMPLE task. */
} nrf_lpcomp_short_mask_t;

#ifdef NRF52_SERIES
/**
 * @enum nrf_lpcomp_hysteresis_t
 * @brief LPCOMP hysteresis.
 */
typedef enum
{
    NRF_LPCOMP_HYST_NOHYST              = LPCOMP_HYST_HYST_NoHyst,      /**< Comparator hysteresis disabled. */
    NRF_LPCOMP_HYST_50mV                = LPCOMP_HYST_HYST_Hyst50mV     /**< Comparator hysteresis enabled (typ. 50 mV). */
}nrf_lpcomp_hysteresis_t;
#endif // NRF52

/** @brief LPCOMP configuration. */
typedef struct
{
    nrf_lpcomp_ref_t            reference; /**< LPCOMP reference. */
    nrf_lpcomp_detect_t         detection; /**< LPCOMP detection type. */
#ifdef NRF52_SERIES
    nrf_lpcomp_hysteresis_t     hyst;      /**< LPCOMP hysteresis. */
#endif // NRF52
} nrf_lpcomp_config_t;

/** Default LPCOMP configuration. */
#define NRF_LPCOMP_CONFIG_DEFAULT { NRF_LPCOMP_REF_SUPPLY_FOUR_EIGHT, NRF_LPCOMP_DETECT_DOWN }

/**
 * @brief Function for configuring LPCOMP.
 *
 * This function powers on LPCOMP and configures it. LPCOMP is in DISABLE state after configuration,
 * so it must be enabled before using it. All shorts are inactive, events are cleared, and LPCOMP is stopped.
 *
 * @param[in] p_config Configuration.
 */
__STATIC_INLINE void nrf_lpcomp_configure(const nrf_lpcomp_config_t * p_config)
{
    NRF_LPCOMP->TASKS_STOP = 1;
    NRF_LPCOMP->ENABLE     = LPCOMP_ENABLE_ENABLE_Disabled << LPCOMP_ENABLE_ENABLE_Pos;
    NRF_LPCOMP->REFSEL     =
        (p_config->reference << LPCOMP_REFSEL_REFSEL_Pos) & LPCOMP_REFSEL_REFSEL_Msk;

    //If external source is choosen extract analog reference index.
    if ((p_config->reference & LPCOMP_REFSEL_REFSEL_ARef)==LPCOMP_REFSEL_REFSEL_ARef)
    {
        uint32_t extref       = p_config->reference >> 16;
        NRF_LPCOMP->EXTREFSEL = (extref << LPCOMP_EXTREFSEL_EXTREFSEL_Pos) & LPCOMP_EXTREFSEL_EXTREFSEL_Msk;
    }

    NRF_LPCOMP->ANADETECT   =
        (p_config->detection << LPCOMP_ANADETECT_ANADETECT_Pos) & LPCOMP_ANADETECT_ANADETECT_Msk;
#ifdef NRF52_SERIES
    NRF_LPCOMP->HYST        = ((p_config->hyst) << LPCOMP_HYST_HYST_Pos) & LPCOMP_HYST_HYST_Msk;
#endif
    NRF_LPCOMP->SHORTS      = 0;
    NRF_LPCOMP->INTENCLR    = LPCOMP_INTENCLR_CROSS_Msk | LPCOMP_INTENCLR_UP_Msk |
                               LPCOMP_INTENCLR_DOWN_Msk | LPCOMP_INTENCLR_READY_Msk;
}


/**
 * @brief Function for selecting the LPCOMP input.
 *
 * This function selects the active input of LPCOMP.
 *
 * @param[in] input Input to be selected.
 */
__STATIC_INLINE void nrf_lpcomp_input_select(nrf_lpcomp_input_t input)
{
    uint32_t lpcomp_enable_state = NRF_LPCOMP->ENABLE;

    NRF_LPCOMP->ENABLE = LPCOMP_ENABLE_ENABLE_Disabled << LPCOMP_ENABLE_ENABLE_Pos;
    NRF_LPCOMP->PSEL   =
        ((uint32_t)input << LPCOMP_PSEL_PSEL_Pos) | (NRF_LPCOMP->PSEL & ~LPCOMP_PSEL_PSEL_Msk);
    NRF_LPCOMP->ENABLE = lpcomp_enable_state;
}


/**
 * @brief Function for enabling the Low Power Comparator.
 *
 * This function enables LPCOMP.
 *
 */
__STATIC_INLINE void nrf_lpcomp_enable(void)
{
    NRF_LPCOMP->ENABLE = LPCOMP_ENABLE_ENABLE_Enabled << LPCOMP_ENABLE_ENABLE_Pos;
    NRF_LPCOMP->EVENTS_READY = 0;
    NRF_LPCOMP->EVENTS_DOWN  = 0;
    NRF_LPCOMP->EVENTS_UP    = 0;
    NRF_LPCOMP->EVENTS_CROSS = 0;
}


/**
 * @brief Function for disabling the Low Power Comparator.
 *
 * This function disables LPCOMP.
 *
 */
__STATIC_INLINE void nrf_lpcomp_disable(void)
{
    NRF_LPCOMP->ENABLE     = LPCOMP_ENABLE_ENABLE_Disabled << LPCOMP_ENABLE_ENABLE_Pos;
}


/**
 * @brief Function for getting the last LPCOMP compare result.
 *
 * @return The last compare result. If 0 then VIN+ < VIN-, if 1 then the opposite.
 */
__STATIC_INLINE uint32_t nrf_lpcomp_result_get(void)
{
    return (uint32_t)NRF_LPCOMP->RESULT;
}


/**
 * @brief Function for enabling interrupts from LPCOMP.
 *
 * @param[in] lpcomp_int_mask Mask of interrupts to be enabled.
 *
 * @sa nrf_lpcomp_int_disable()
 * @sa nrf_lpcomp_int_enable_check()
 */
__STATIC_INLINE void nrf_lpcomp_int_enable(uint32_t lpcomp_int_mask)
{
    NRF_LPCOMP->INTENSET = lpcomp_int_mask;
}


/**
 * @brief Function for disabling interrupts from LPCOMP.
 *
 * @param[in] lpcomp_int_mask Mask of interrupts to be disabled.
 *
 * @sa nrf_lpcomp_int_enable()
 * @sa nrf_lpcomp_int_enable_check()
 */
__STATIC_INLINE void nrf_lpcomp_int_disable(uint32_t lpcomp_int_mask)
{
    NRF_LPCOMP->INTENCLR = lpcomp_int_mask;
}


/**
 * @brief Function for getting the enabled interrupts of LPCOMP.
 *
 * @param[in] lpcomp_int_mask Mask of interrupts to be checked.
 *
 * @retval true If any of interrupts of the specified mask are enabled.
 *
 * @sa nrf_lpcomp_int_enable()
 * @sa nrf_lpcomp_int_disable()
 */
__STATIC_INLINE bool nrf_lpcomp_int_enable_check(uint32_t lpcomp_int_mask)
{
    return (NRF_LPCOMP->INTENSET & lpcomp_int_mask); // when read this register will return the value of INTEN.
}


/**
 * @brief Function for getting the address of a specific LPCOMP task register.
 *
 * @param[in] lpcomp_task LPCOMP task.
 *
 * @return The address of the specified LPCOMP task.
 */
__STATIC_INLINE uint32_t * nrf_lpcomp_task_address_get(nrf_lpcomp_task_t lpcomp_task)
{
    return (uint32_t *)((uint8_t *)NRF_LPCOMP + lpcomp_task);
}


/**
 * @brief Function for getting the address of a specific LPCOMP event register.
 *
 * @param[in] lpcomp_event LPCOMP event.
 *
 * @return The address of the specified LPCOMP event.
 */
__STATIC_INLINE uint32_t * nrf_lpcomp_event_address_get(nrf_lpcomp_event_t lpcomp_event)
{
    return (uint32_t *)((uint8_t *)NRF_LPCOMP + lpcomp_event);
}


/**
 * @brief  Function for setting LPCOMP shorts.
 *
 * @param[in] lpcomp_short_mask LPCOMP shorts by mask.
 *
 */
__STATIC_INLINE void nrf_lpcomp_shorts_enable(uint32_t lpcomp_short_mask)
{
    NRF_LPCOMP->SHORTS |= lpcomp_short_mask;
}


/**
 * @brief Function for clearing LPCOMP shorts by mask.
 *
 * @param[in] lpcomp_short_mask LPCOMP shorts to be cleared.
 *
 */
__STATIC_INLINE void nrf_lpcomp_shorts_disable(uint32_t lpcomp_short_mask)
{
    NRF_LPCOMP->SHORTS &= ~lpcomp_short_mask;
}


/**
 * @brief Function for setting a specific LPCOMP task.
 *
 * @param[in] lpcomp_task LPCOMP task to be set.
 *
 */
__STATIC_INLINE void nrf_lpcomp_task_trigger(nrf_lpcomp_task_t lpcomp_task)
{
    *( (volatile uint32_t *)( (uint8_t *)NRF_LPCOMP + lpcomp_task) ) = 1;
}


/**
 * @brief Function for clearing a specific LPCOMP event.
 *
 * @param[in] lpcomp_event LPCOMP event to be cleared.
 *
 */
__STATIC_INLINE void nrf_lpcomp_event_clear(nrf_lpcomp_event_t lpcomp_event)
{
    *( (volatile uint32_t *)( (uint8_t *)NRF_LPCOMP + lpcomp_event) ) = 0;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_LPCOMP + lpcomp_event));
    (void)dummy;
#endif
}


/**
 * @brief Function for getting the state of a specific LPCOMP event.
 *
 * @retval true If the specified LPCOMP event is active.
 *
 */
__STATIC_INLINE bool nrf_lpcomp_event_check(nrf_lpcomp_event_t lpcomp_event)
{
    return (bool) (*(volatile uint32_t *)( (uint8_t *)NRF_LPCOMP + lpcomp_event));
}


/**
 *@}
 **/


#ifdef __cplusplus
}
#endif

#endif /* NRF_LPCOMP_H_ */
