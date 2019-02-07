/*
 * Copyright (c) 2015 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_COMP_H_
#define NRF_COMP_H_

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_comp_hal COMP HAL
 * @{
 * @ingroup nrf_comp
 * @brief   Hardware access layer for managing the Comparator (COMP) peripheral.
 */

/**
 * @enum nrf_comp_input_t
 * @brief COMP analog pin selection.
 */
typedef enum
{
    NRF_COMP_INPUT_0 = COMP_PSEL_PSEL_AnalogInput0,        /*!< AIN0 selected as analog input. */
    NRF_COMP_INPUT_1 = COMP_PSEL_PSEL_AnalogInput1,        /*!< AIN1 selected as analog input. */
    NRF_COMP_INPUT_2 = COMP_PSEL_PSEL_AnalogInput2,        /*!< AIN2 selected as analog input. */
    NRF_COMP_INPUT_3 = COMP_PSEL_PSEL_AnalogInput3,        /*!< AIN3 selected as analog input. */
    NRF_COMP_INPUT_4 = COMP_PSEL_PSEL_AnalogInput4,        /*!< AIN4 selected as analog input. */
    NRF_COMP_INPUT_5 = COMP_PSEL_PSEL_AnalogInput5,        /*!< AIN5 selected as analog input. */
    NRF_COMP_INPUT_6 = COMP_PSEL_PSEL_AnalogInput6,        /*!< AIN6 selected as analog input. */
#if defined (COMP_PSEL_PSEL_AnalogInput7) || defined (__NRFX_DOXYGEN__)
    NRF_COMP_INPUT_7 = COMP_PSEL_PSEL_AnalogInput7,        /*!< AIN7 selected as analog input. */
#endif
#if defined (COMP_PSEL_PSEL_VddDiv2) || defined (__NRFX_DOXYGEN__)
    NRF_COMP_VDD_DIV2 = COMP_PSEL_PSEL_VddDiv2,            /*!< VDD/2 selected as analog input. */
#endif
}nrf_comp_input_t;

/**
 * @enum nrf_comp_ref_t
 * @brief COMP reference selection.
 */
typedef enum
{
    NRF_COMP_REF_Int1V2 = COMP_REFSEL_REFSEL_Int1V2,     /*!< VREF = internal 1.2 V reference (VDD >= 1.7 V). */
    NRF_COMP_REF_Int1V8 = COMP_REFSEL_REFSEL_Int1V8,     /*!< VREF = internal 1.8 V reference (VDD >= VREF + 0.2 V). */
    NRF_COMP_REF_Int2V4 = COMP_REFSEL_REFSEL_Int2V4,     /*!< VREF = internal 2.4 V reference (VDD >= VREF + 0.2 V). */
    NRF_COMP_REF_VDD = COMP_REFSEL_REFSEL_VDD,           /*!< VREF = VDD. */
    NRF_COMP_REF_ARef = COMP_REFSEL_REFSEL_ARef          /*!< VREF = AREF (VDD >= VREF >= AREFMIN). */
}nrf_comp_ref_t;

/**
 * @enum nrf_comp_ext_ref_t
 * @brief COMP external analog reference selection.
 */
typedef enum
{
    NRF_COMP_EXT_REF_0 = COMP_EXTREFSEL_EXTREFSEL_AnalogReference0,        /*!< Use AIN0 as external analog reference. */
    NRF_COMP_EXT_REF_1 = COMP_EXTREFSEL_EXTREFSEL_AnalogReference1         /*!< Use AIN1 as external analog reference. */
}nrf_comp_ext_ref_t;

/**
 * @brief COMP THDOWN and THUP values that are used to calculate the threshold voltages VDOWN and VUP.
 */
typedef struct
{
    uint8_t th_down; /*!< THDOWN value. */
    uint8_t th_up;   /*!< THUP value. */
}nrf_comp_th_t;

/**
 * @enum nrf_comp_main_mode_t
 * @brief COMP main operation mode.
 */
typedef enum
{
    NRF_COMP_MAIN_MODE_SE = COMP_MODE_MAIN_SE,        /*!< Single ended mode. */
    NRF_COMP_MAIN_MODE_Diff = COMP_MODE_MAIN_Diff     /*!< Differential mode. */
}nrf_comp_main_mode_t;

/**
 * @enum nrf_comp_sp_mode_t
 * @brief COMP speed and power mode.
 */
typedef enum
{
    NRF_COMP_SP_MODE_Low = COMP_MODE_SP_Low,          /*!< Low power mode. */
    NRF_COMP_SP_MODE_Normal = COMP_MODE_SP_Normal,    /*!< Normal mode. */
    NRF_COMP_SP_MODE_High = COMP_MODE_SP_High         /*!< High speed mode. */
}nrf_comp_sp_mode_t;

/**
 * @enum nrf_comp_hyst_t
 * @brief COMP comparator hysteresis.
 */
typedef enum
{
    NRF_COMP_HYST_NoHyst = COMP_HYST_HYST_NoHyst,    /*!< Comparator hysteresis disabled. */
    NRF_COMP_HYST_50mV = COMP_HYST_HYST_Hyst50mV     /*!< Comparator hysteresis enabled. */
}nrf_comp_hyst_t;

#if defined (COMP_ISOURCE_ISOURCE_Msk) || defined (__NRFX_DOXYGEN__)
/**
 * @brief COMP current source selection on analog input.
 */
typedef enum
{
    NRF_COMP_ISOURCE_Off = COMP_ISOURCE_ISOURCE_Off,            /*!< Current source disabled. */
    NRF_COMP_ISOURCE_Ien2uA5 = COMP_ISOURCE_ISOURCE_Ien2mA5,    /*!< Current source enabled (+/- 2.5 uA). */
    NRF_COMP_ISOURCE_Ien5uA = COMP_ISOURCE_ISOURCE_Ien5mA,      /*!< Current source enabled (+/- 5 uA). */
    NRF_COMP_ISOURCE_Ien10uA = COMP_ISOURCE_ISOURCE_Ien10mA     /*!< Current source enabled (+/- 10 uA). */
}nrf_isource_t;
#endif

/**
 * @enum nrf_comp_task_t
 * @brief COMP tasks.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_COMP_TASK_START  = offsetof(NRF_COMP_Type, TASKS_START), /*!< COMP start sampling task. */
    NRF_COMP_TASK_STOP   = offsetof(NRF_COMP_Type, TASKS_STOP),  /*!< COMP stop sampling task. */
    NRF_COMP_TASK_SAMPLE = offsetof(NRF_COMP_Type, TASKS_SAMPLE) /*!< Sample comparator value. */
    /*lint -restore*/
}nrf_comp_task_t;

/**
 * @enum nrf_comp_event_t
 * @brief COMP events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_COMP_EVENT_READY = offsetof(NRF_COMP_Type, EVENTS_READY), /*!< COMP is ready and output is valid. */
    NRF_COMP_EVENT_DOWN  = offsetof(NRF_COMP_Type, EVENTS_DOWN),  /*!< Input voltage crossed the threshold going down. */
    NRF_COMP_EVENT_UP    = offsetof(NRF_COMP_Type, EVENTS_UP),    /*!< Input voltage crossed the threshold going up. */
    NRF_COMP_EVENT_CROSS = offsetof(NRF_COMP_Type, EVENTS_CROSS)  /*!< Input voltage crossed the threshold in any direction. */
    /*lint -restore*/
}nrf_comp_event_t;

/**
 * @brief COMP reference configuration.
 */
typedef struct
{
    nrf_comp_ref_t     reference;        /*!< COMP reference selection. */
    nrf_comp_ext_ref_t external;         /*!< COMP external analog reference selection. */
}nrf_comp_ref_conf_t;


/**
 * @brief Function for enabling the COMP peripheral.
 */
__STATIC_INLINE void nrf_comp_enable(void);


/**
 * @brief Function for disabling the COMP peripheral.
 */

__STATIC_INLINE void nrf_comp_disable(void);

/**
 * @brief Function for checking if the COMP peripheral is enabled.
 *
 * @retval true  If the COMP peripheral is enabled.
 * @retval false If the COMP peripheral is not enabled.
 */
__STATIC_INLINE bool nrf_comp_enable_check(void);

/**
 * @brief Function for setting the reference source.
 *
 * @param[in] reference                 COMP reference selection.
 */
__STATIC_INLINE void nrf_comp_ref_set(nrf_comp_ref_t reference);


/**
 * @brief Function for setting the external analog reference source.
 *
 * @param[in] ext_ref                   COMP external analog reference selection.
 */
__STATIC_INLINE void nrf_comp_ext_ref_set(nrf_comp_ext_ref_t ext_ref);


/**
 * @brief Function for setting threshold voltages.
 *
 * @param[in] threshold                 COMP VDOWN and VUP thresholds.
 */
__STATIC_INLINE void nrf_comp_th_set(nrf_comp_th_t threshold);


/**
 * @brief Function for setting the main mode.
 *
 * @param[in] main_mode                 COMP main operation mode.
 */
__STATIC_INLINE void nrf_comp_main_mode_set(nrf_comp_main_mode_t main_mode);


/**
 * @brief Function for setting the speed mode.
 *
 * @param[in] speed_mode                COMP speed and power mode.
 */
__STATIC_INLINE void nrf_comp_speed_mode_set(nrf_comp_sp_mode_t speed_mode);


/**
 * @brief Function for setting the hysteresis.
 *
 * @param[in] hyst                      COMP comparator hysteresis.
 */
__STATIC_INLINE void nrf_comp_hysteresis_set(nrf_comp_hyst_t hyst);


#if defined (COMP_ISOURCE_ISOURCE_Msk) || defined (__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the current source on the analog input.
 *
 * @param[in] isource                   COMP current source selection on analog input.
 */
__STATIC_INLINE void nrf_comp_isource_set(nrf_isource_t isource);
#endif


/**
 * @brief Function for selecting the active input of the COMP.
 *
 * @param[in] input Input to be selected.
 */
__STATIC_INLINE void nrf_comp_input_select(nrf_comp_input_t input);


/**
 * @brief Function for getting the last COMP compare result.
 *
 * @return The last compare result. If 0, then VIN+ < VIN-. If 1, then VIN+ > VIN-.
 *
 * @note If VIN+ == VIN-, the return value depends on the previous result.
 */
__STATIC_INLINE uint32_t nrf_comp_result_get(void);


/**
 * @brief Function for enabling interrupts from COMP.
 *
 * @param[in] comp_int_mask Mask of interrupts to be enabled.
 *
 * @sa nrf_comp_int_enable_check()
 */
__STATIC_INLINE void nrf_comp_int_enable(uint32_t comp_int_mask);

/**
 * @brief Function for disabling interrupts from COMP.
 *
 * @param[in] comp_int_mask Mask of interrupts to be disabled.
 *
 * @sa nrf_comp_int_enable_check()
 */
__STATIC_INLINE void nrf_comp_int_disable(uint32_t comp_int_mask);


/**
 * @brief Function for getting the enabled interrupts of COMP.
 *
 * @param[in] comp_int_mask Mask of interrupts to be checked.
 *
 * @retval true If any interrupts of the specified mask are enabled.
 */
__STATIC_INLINE bool nrf_comp_int_enable_check(uint32_t comp_int_mask);



/**
 * @brief Function for getting the address of a specific COMP task register.
 *
 * @param[in] comp_task COMP task.
 *
 * @return Address of the specified COMP task.
 */
__STATIC_INLINE uint32_t * nrf_comp_task_address_get(nrf_comp_task_t comp_task);


/**
 * @brief Function for getting the address of a specific COMP event register.
 *
 * @param[in] comp_event COMP event.
 *
 * @return Address of the specified COMP event.
 */
__STATIC_INLINE uint32_t * nrf_comp_event_address_get(nrf_comp_event_t comp_event);


/**
 * @brief  Function for setting COMP shorts.
 *
 * @param[in] comp_short_mask COMP shorts by mask.
 *
 */
__STATIC_INLINE void nrf_comp_shorts_enable(uint32_t comp_short_mask);


/**
 * @brief Function for clearing COMP shorts by mask.
 *
 * @param[in] comp_short_mask COMP shorts to be cleared.
 *
 */
__STATIC_INLINE void nrf_comp_shorts_disable(uint32_t comp_short_mask);


/**
 * @brief Function for setting a specific COMP task.
 *
 * @param[in] comp_task COMP task to be set.
 *
 */
__STATIC_INLINE void nrf_comp_task_trigger(nrf_comp_task_t comp_task);


/**
 * @brief Function for clearing a specific COMP event.
 *
 * @param[in] comp_event COMP event to be cleared.
 *
 */
__STATIC_INLINE void nrf_comp_event_clear(nrf_comp_event_t comp_event);


/**
 * @brief Function for getting the state of a specific COMP event.
 *
 * @retval true If the specified COMP event is active.
 *
 */
__STATIC_INLINE bool nrf_comp_event_check(nrf_comp_event_t comp_event);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_comp_enable(void)
{
    NRF_COMP->ENABLE = (COMP_ENABLE_ENABLE_Enabled << COMP_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_comp_disable(void)
{
    NRF_COMP->ENABLE = (COMP_ENABLE_ENABLE_Disabled << COMP_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE bool nrf_comp_enable_check(void)
{
    return ((NRF_COMP->ENABLE) & COMP_ENABLE_ENABLE_Enabled);
}

__STATIC_INLINE void nrf_comp_ref_set(nrf_comp_ref_t reference)
{
    NRF_COMP->REFSEL = (reference << COMP_REFSEL_REFSEL_Pos);
}

__STATIC_INLINE void nrf_comp_ext_ref_set(nrf_comp_ext_ref_t ext_ref)
{
    NRF_COMP->EXTREFSEL = (ext_ref << COMP_EXTREFSEL_EXTREFSEL_Pos);
}

__STATIC_INLINE void nrf_comp_th_set(nrf_comp_th_t threshold)
{
    NRF_COMP->TH =
        ((threshold.th_down << COMP_TH_THDOWN_Pos) & COMP_TH_THDOWN_Msk) |
        ((threshold.th_up << COMP_TH_THUP_Pos) & COMP_TH_THUP_Msk);
}

__STATIC_INLINE void nrf_comp_main_mode_set(nrf_comp_main_mode_t main_mode)
{
    NRF_COMP->MODE |= (main_mode << COMP_MODE_MAIN_Pos);
}

__STATIC_INLINE void nrf_comp_speed_mode_set(nrf_comp_sp_mode_t speed_mode)
{
    NRF_COMP->MODE |= (speed_mode << COMP_MODE_SP_Pos);
}

__STATIC_INLINE void nrf_comp_hysteresis_set(nrf_comp_hyst_t hyst)
{
    NRF_COMP->HYST = (hyst << COMP_HYST_HYST_Pos) & COMP_HYST_HYST_Msk;
}

#if defined (COMP_ISOURCE_ISOURCE_Msk)
__STATIC_INLINE void nrf_comp_isource_set(nrf_isource_t isource)
{
    NRF_COMP->ISOURCE = (isource << COMP_ISOURCE_ISOURCE_Pos) & COMP_ISOURCE_ISOURCE_Msk;
}
#endif

__STATIC_INLINE void nrf_comp_input_select(nrf_comp_input_t input)
{
    NRF_COMP->PSEL   = ((uint32_t)input << COMP_PSEL_PSEL_Pos);
}

__STATIC_INLINE uint32_t nrf_comp_result_get(void)
{
    return (uint32_t)NRF_COMP->RESULT;
}

__STATIC_INLINE void nrf_comp_int_enable(uint32_t comp_int_mask)
{
    NRF_COMP->INTENSET = comp_int_mask;
}

__STATIC_INLINE void nrf_comp_int_disable(uint32_t comp_int_mask)
{
    NRF_COMP->INTENCLR = comp_int_mask;
}

__STATIC_INLINE bool nrf_comp_int_enable_check(uint32_t comp_int_mask)
{
    return (NRF_COMP->INTENSET & comp_int_mask); // when read this register will return the value of INTEN.
}

__STATIC_INLINE uint32_t * nrf_comp_task_address_get(nrf_comp_task_t comp_task)
{
    return (uint32_t *)((uint8_t *)NRF_COMP + (uint32_t)comp_task);
}

__STATIC_INLINE uint32_t * nrf_comp_event_address_get(nrf_comp_event_t comp_event)
{
    return (uint32_t *)((uint8_t *)NRF_COMP + (uint32_t)comp_event);
}

__STATIC_INLINE void nrf_comp_shorts_enable(uint32_t comp_short_mask)
{
    NRF_COMP->SHORTS |= comp_short_mask;
}

__STATIC_INLINE void nrf_comp_shorts_disable(uint32_t comp_short_mask)
{
    NRF_COMP->SHORTS &= ~comp_short_mask;
}

__STATIC_INLINE void nrf_comp_task_trigger(nrf_comp_task_t comp_task)
{
    *( (volatile uint32_t *)( (uint8_t *)NRF_COMP + comp_task) ) = 1;
}

__STATIC_INLINE void nrf_comp_event_clear(nrf_comp_event_t comp_event)
{
    *( (volatile uint32_t *)( (uint8_t *)NRF_COMP + (uint32_t)comp_event) ) = 0;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_COMP + (uint32_t)comp_event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_comp_event_check(nrf_comp_event_t comp_event)
{
    return (bool) (*(volatile uint32_t *)( (uint8_t *)NRF_COMP + comp_event));
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_COMP_H_
