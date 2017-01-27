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

#ifndef NRF_ADC_H_
#define NRF_ADC_H_

/**
 * @defgroup nrf_adc_hal ADC HAL
 * @{
 * @ingroup nrf_adc
 * @brief @tagAPI51 Hardware access layer for managing the analog-to-digital converter (ADC).
 */

#include <stdbool.h>
#include <stddef.h>

#include "nrf_peripherals.h"
#include "nrf.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ADC_PRESENT) || defined(__SDK_DOXYGEN__)
/**
 * @enum  nrf_adc_config_resolution_t
 * @brief Resolution of the analog-to-digital converter.
 */

/**
 * @brief ADC interrupts.
 */
typedef enum
{
    NRF_ADC_INT_END_MASK  = ADC_INTENSET_END_Msk,   /**< ADC interrupt on END event. */
} nrf_adc_int_mask_t;

typedef enum
{
    NRF_ADC_CONFIG_RES_8BIT  = ADC_CONFIG_RES_8bit,  /**< 8 bit resolution. */
    NRF_ADC_CONFIG_RES_9BIT  = ADC_CONFIG_RES_9bit,  /**< 9 bit resolution. */
    NRF_ADC_CONFIG_RES_10BIT = ADC_CONFIG_RES_10bit, /**< 10 bit resolution. */
} nrf_adc_config_resolution_t;


/**
 * @enum nrf_adc_config_scaling_t
 * @brief Scaling factor of the analog-to-digital conversion.
 */
typedef enum
{
    NRF_ADC_CONFIG_SCALING_INPUT_FULL_SCALE  = ADC_CONFIG_INPSEL_AnalogInputNoPrescaling,        /**< Full scale input. */
    NRF_ADC_CONFIG_SCALING_INPUT_TWO_THIRDS  = ADC_CONFIG_INPSEL_AnalogInputTwoThirdsPrescaling, /**< 2/3 scale input. */
    NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD   = ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling,  /**< 1/3 scale input. */
    NRF_ADC_CONFIG_SCALING_SUPPLY_TWO_THIRDS = ADC_CONFIG_INPSEL_SupplyTwoThirdsPrescaling,      /**< 2/3 of supply. */
    NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD  = ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling        /**< 1/3 of supply. */
} nrf_adc_config_scaling_t;

/**
 * @enum nrf_adc_config_reference_t
 * @brief Reference selection of the analog-to-digital converter.
 */
typedef enum
{
    NRF_ADC_CONFIG_REF_VBG              = ADC_CONFIG_REFSEL_VBG,                      /**< 1.2 V reference. */
    NRF_ADC_CONFIG_REF_SUPPLY_ONE_HALF  = ADC_CONFIG_REFSEL_SupplyOneHalfPrescaling,  /**< 1/2 of power supply. */
    NRF_ADC_CONFIG_REF_SUPPLY_ONE_THIRD = ADC_CONFIG_REFSEL_SupplyOneThirdPrescaling, /**< 1/3 of power supply. */
    NRF_ADC_CONFIG_REF_EXT_REF0         = ADC_CONFIG_REFSEL_External |
                                          ADC_CONFIG_EXTREFSEL_AnalogReference0 <<
    ADC_CONFIG_EXTREFSEL_Pos, /**< External reference 0. */
        NRF_ADC_CONFIG_REF_EXT_REF1 = ADC_CONFIG_REFSEL_External |
                                  ADC_CONFIG_EXTREFSEL_AnalogReference1 << ADC_CONFIG_EXTREFSEL_Pos, /**< External reference 0. */
} nrf_adc_config_reference_t;

/**
 * @enum nrf_adc_config_input_t
 * @brief Input selection of the analog-to-digital converter.
 */
typedef enum
{
    NRF_ADC_CONFIG_INPUT_DISABLED = ADC_CONFIG_PSEL_Disabled,     /**< No input selected. */
    NRF_ADC_CONFIG_INPUT_0        = ADC_CONFIG_PSEL_AnalogInput0, /**< Input 0. */
    NRF_ADC_CONFIG_INPUT_1        = ADC_CONFIG_PSEL_AnalogInput1, /**< Input 1. */
    NRF_ADC_CONFIG_INPUT_2        = ADC_CONFIG_PSEL_AnalogInput2, /**< Input 2. */
    NRF_ADC_CONFIG_INPUT_3        = ADC_CONFIG_PSEL_AnalogInput3, /**< Input 3. */
    NRF_ADC_CONFIG_INPUT_4        = ADC_CONFIG_PSEL_AnalogInput4, /**< Input 4. */
    NRF_ADC_CONFIG_INPUT_5        = ADC_CONFIG_PSEL_AnalogInput5, /**< Input 5. */
    NRF_ADC_CONFIG_INPUT_6        = ADC_CONFIG_PSEL_AnalogInput6, /**< Input 6. */
    NRF_ADC_CONFIG_INPUT_7        = ADC_CONFIG_PSEL_AnalogInput7, /**< Input 7. */
} nrf_adc_config_input_t;

/**
 * @enum nrf_adc_task_t
 * @brief Analog-to-digital converter tasks.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_ADC_TASK_START = offsetof(NRF_ADC_Type, TASKS_START), /**< ADC start sampling task. */
    NRF_ADC_TASK_STOP  = offsetof(NRF_ADC_Type, TASKS_STOP)   /**< ADC stop sampling task. */
    /*lint -restore*/
} nrf_adc_task_t;

/**
 * @enum nrf_adc_event_t
 * @brief Analog-to-digital converter events.
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    /*lint -save -e30*/
    NRF_ADC_EVENT_END = offsetof(NRF_ADC_Type, EVENTS_END) /**< End of conversion event. */
    /*lint -restore*/
} nrf_adc_event_t;

/**@brief Analog-to-digital converter configuration. */
typedef struct
{
    nrf_adc_config_resolution_t resolution; /**< ADC resolution. */
    nrf_adc_config_scaling_t    scaling;    /**< ADC scaling factor. */
    nrf_adc_config_reference_t  reference;  /**< ADC reference. */
} nrf_adc_config_t;

/** Default ADC configuration. */
#define NRF_ADC_CONFIG_DEFAULT { NRF_ADC_CONFIG_RES_10BIT,               \
                                 NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD, \
                                 NRF_ADC_CONFIG_REF_VBG }

/**
 * @brief Function for configuring ADC.
 *
 * This function powers on the analog-to-digital converter and configures it.
 * After the configuration, the ADC is in DISABLE state and must be
 * enabled before using it.
 *
 * @param[in] config Configuration parameters.
 */
void nrf_adc_configure(nrf_adc_config_t * config);

/**
 * @brief Blocking function for executing a single ADC conversion.
 *
 * This function selects the desired input, starts a single conversion,
 * waits for it to finish, and returns the result.
 * After the input is selected, the analog-to-digital converter
 * is left in STOP state.
 * The function does not check if the ADC is initialized and powered.
 *
 * @param[in] input Input to be selected.
 *
 * @return Conversion result.
 */
int32_t nrf_adc_convert_single(nrf_adc_config_input_t input);

/**
 * @brief Function for selecting ADC input.
 *
 * This function selects the active input of ADC. Ensure that
 * the ADC is powered on and in IDLE state before calling this function.
 *
 * @param[in] input Input to be selected.
 */
__STATIC_INLINE void nrf_adc_input_select(nrf_adc_config_input_t input)
{
    NRF_ADC->CONFIG =
        ((uint32_t)input << ADC_CONFIG_PSEL_Pos) | (NRF_ADC->CONFIG & ~ADC_CONFIG_PSEL_Msk);

    if (input != NRF_ADC_CONFIG_INPUT_DISABLED)
    {
        NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled << ADC_ENABLE_ENABLE_Pos;
    }
    else
    {
        NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Disabled << ADC_ENABLE_ENABLE_Pos;
    }
}


/**
 * @brief Function for retrieving the ADC conversion result.
 *
 * This function retrieves and returns the last analog-to-digital conversion result.
 *
 * @return Last conversion result.
 */
__STATIC_INLINE int32_t nrf_adc_result_get(void)
{
    return (int32_t)NRF_ADC->RESULT;
}


/**
 * @brief Function for checking whether the ADC is busy.
 *
 * This function checks whether the analog-to-digital converter is busy with a conversion.
 *
 * @retval true If the ADC is busy.
 * @retval false If the ADC is not busy.
 */
__STATIC_INLINE bool nrf_adc_is_busy(void)
{
    return ( (NRF_ADC->BUSY & ADC_BUSY_BUSY_Msk) == ADC_BUSY_BUSY_Msk);
}

/**
 * @brief Function for getting the ADC's enabled interrupts.
 *
 * @param[in] mask Mask of interrupts to check.
 *
 * @return State of the interrupts selected by the mask.
 *
 * @sa nrf_adc_int_enable()
 * @sa nrf_adc_int_disable()
 */
__STATIC_INLINE uint32_t nrf_adc_int_get(uint32_t mask)
{
    return (NRF_ADC->INTENSET & mask); // when read this register will return the value of INTEN.
}


/**
 * @brief Function for starting conversion.
 *
 * @sa nrf_adc_stop()
 *
 */
__STATIC_INLINE void nrf_adc_start(void)
{
    NRF_ADC->TASKS_START = 1;
}


/**
 * @brief Function for stopping conversion.
 *
 * If the analog-to-digital converter is in inactive state, power consumption is reduced.
 *
 * @sa nrf_adc_start()
 *
 */
__STATIC_INLINE void nrf_adc_stop(void)
{
    NRF_ADC->TASKS_STOP = 1;
}


/**
 * @brief Function for checking if the requested ADC conversion has ended.
 *
 * @retval true If the task has finished.
 * @retval false If the task is still running.
 */
__STATIC_INLINE bool nrf_adc_conversion_finished(void)
{
    return ((bool)NRF_ADC->EVENTS_END);
}

/**
 * @brief Function for clearing the conversion END event.
 */
__STATIC_INLINE void nrf_adc_conversion_event_clean(void)
{
    NRF_ADC->EVENTS_END = 0;
}

/**
 * @brief Function for getting the address of an ADC task register.
 *
 * @param[in] adc_task ADC task.
 *
 * @return Address of the specified ADC task.
 */
__STATIC_INLINE uint32_t nrf_adc_task_address_get(nrf_adc_task_t adc_task);

/**
 * @brief Function for getting the address of a specific ADC event register.
 *
 * @param[in] adc_event ADC event.
 *
 * @return Address of the specified ADC event.
 */
__STATIC_INLINE uint32_t nrf_adc_event_address_get(nrf_adc_event_t adc_event);

/**
 * @brief Function for setting the CONFIG register in ADC.
 *
 * @param[in] configuration Value to be written to the CONFIG register.
 */
__STATIC_INLINE void nrf_adc_config_set(uint32_t configuration);

/**
 * @brief Function for clearing an ADC event.
 *
 * @param[in] event Event to clear.
 */
__STATIC_INLINE void nrf_adc_event_clear(nrf_adc_event_t event);

/**
 * @brief Function for checking state of an ADC event.
 *
 * @param[in] event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_adc_event_check(nrf_adc_event_t event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] int_mask  Interrupts to enable.
 */
__STATIC_INLINE void nrf_adc_int_enable(uint32_t int_mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] int_mask  Interrupts to disable.
 */
__STATIC_INLINE void nrf_adc_int_disable(uint32_t int_mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] int_mask Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_adc_int_enable_check(nrf_adc_int_mask_t int_mask);

/**
 * @brief Function for activating a specific ADC task.
 *
 * @param[in] task Task to activate.
 */
__STATIC_INLINE void nrf_adc_task_trigger(nrf_adc_task_t task);

/**
 * @brief Function for enabling ADC.
 *
 */
__STATIC_INLINE void nrf_adc_enable(void);

/**
 * @brief Function for disabling ADC.
 *
 */
__STATIC_INLINE void nrf_adc_disable(void);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE uint32_t nrf_adc_task_address_get(nrf_adc_task_t adc_task)
{
    return (uint32_t)((uint8_t *)NRF_ADC + adc_task);
}

__STATIC_INLINE uint32_t nrf_adc_event_address_get(nrf_adc_event_t adc_event)
{
    return (uint32_t)((uint8_t *)NRF_ADC + adc_event);
}

__STATIC_INLINE void nrf_adc_config_set(uint32_t configuration)
{
    NRF_ADC->CONFIG = configuration;
}

__STATIC_INLINE void nrf_adc_event_clear(nrf_adc_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_ADC + (uint32_t)event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_ADC + (uint32_t)event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_adc_event_check(nrf_adc_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)NRF_ADC + (uint32_t)event);
}

__STATIC_INLINE void nrf_adc_int_enable(uint32_t int_mask)
{
    NRF_ADC->INTENSET = int_mask;
}

__STATIC_INLINE void nrf_adc_int_disable(uint32_t int_mask)
{
    NRF_ADC->INTENCLR = int_mask;
}

__STATIC_INLINE bool nrf_adc_int_enable_check(nrf_adc_int_mask_t int_mask)
{
    return (bool)(NRF_ADC->INTENSET & int_mask);
}

__STATIC_INLINE void nrf_adc_task_trigger(nrf_adc_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_ADC + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE void nrf_adc_enable(void)
{
    NRF_ADC->ENABLE = 1;
}

__STATIC_INLINE void nrf_adc_disable(void)
{
    NRF_ADC->ENABLE = 0;
}
#endif
#endif /* ADC_PRESENT */
/**
 *@}
 **/


#ifdef __cplusplus
}
#endif

#endif /* NRF_ADC_H_ */
