/*
 * Copyright (c) 2014 - 2018, Nordic Semiconductor ASA
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

#ifndef NRF_ADC_H_
#define NRF_ADC_H_

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_adc_hal ADC HAL
 * @{
 * @ingroup nrf_adc
 * @brief   Hardware access layer for managing the Analog-to-Digital Converter (ADC)
 *          peripheral.
 */

/** @brief ADC interrupts. */
typedef enum
{
    NRF_ADC_INT_END_MASK  = ADC_INTENSET_END_Msk,   /**< ADC interrupt on END event. */
} nrf_adc_int_mask_t;

/** @brief Resolution of the analog-to-digital converter. */
typedef enum
{
    NRF_ADC_CONFIG_RES_8BIT  = ADC_CONFIG_RES_8bit,  /**< 8-bit resolution. */
    NRF_ADC_CONFIG_RES_9BIT  = ADC_CONFIG_RES_9bit,  /**< 9-bit resolution. */
    NRF_ADC_CONFIG_RES_10BIT = ADC_CONFIG_RES_10bit, /**< 10-bit resolution. */
} nrf_adc_config_resolution_t;


/** @brief Scaling factor of the analog-to-digital conversion. */
typedef enum
{
    NRF_ADC_CONFIG_SCALING_INPUT_FULL_SCALE  = ADC_CONFIG_INPSEL_AnalogInputNoPrescaling,        /**< Full scale input. */
    NRF_ADC_CONFIG_SCALING_INPUT_TWO_THIRDS  = ADC_CONFIG_INPSEL_AnalogInputTwoThirdsPrescaling, /**< 2/3 scale input. */
    NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD   = ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling,  /**< 1/3 scale input. */
    NRF_ADC_CONFIG_SCALING_SUPPLY_TWO_THIRDS = ADC_CONFIG_INPSEL_SupplyTwoThirdsPrescaling,      /**< 2/3 of supply. */
    NRF_ADC_CONFIG_SCALING_SUPPLY_ONE_THIRD  = ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling        /**< 1/3 of supply. */
} nrf_adc_config_scaling_t;


/**
 * @brief External reference selection of the analog-to-digital converter.
 */
typedef enum
{
    NRF_ADC_CONFIG_EXTREFSEL_NONE  = ADC_CONFIG_EXTREFSEL_None,             /**< Analog reference inputs disabled. */
    NRF_ADC_CONFIG_EXTREFSEL_AREF0 = ADC_CONFIG_EXTREFSEL_AnalogReference0, /**< AREF0 as analog reference. */
    NRF_ADC_CONFIG_EXTREFSEL_AREF1 = ADC_CONFIG_EXTREFSEL_AnalogReference1  /**< AREF1 as analog reference. */
} nrf_adc_config_extref_t;

/**
 * @brief Reference selection of the analog-to-digital converter.
 */
typedef enum
{
    NRF_ADC_CONFIG_REF_VBG              = ADC_CONFIG_REFSEL_VBG,                      /**< 1.2 V reference. */
    NRF_ADC_CONFIG_REF_SUPPLY_ONE_HALF  = ADC_CONFIG_REFSEL_SupplyOneHalfPrescaling,  /**< 1/2 of power supply. */
    NRF_ADC_CONFIG_REF_SUPPLY_ONE_THIRD = ADC_CONFIG_REFSEL_SupplyOneThirdPrescaling, /**< 1/3 of power supply. */
    NRF_ADC_CONFIG_REF_EXT              = ADC_CONFIG_REFSEL_External                  /**< External reference. See @ref nrf_adc_config_extref_t for further configuration.*/
} nrf_adc_config_reference_t;

/** @brief Input selection of the analog-to-digital converter. */
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

/** @brief Analog-to-digital converter tasks. */
typedef enum
{
    /*lint -save -e30*/
    NRF_ADC_TASK_START = offsetof(NRF_ADC_Type, TASKS_START), /**< ADC start sampling task. */
    NRF_ADC_TASK_STOP  = offsetof(NRF_ADC_Type, TASKS_STOP)   /**< ADC stop sampling task. */
    /*lint -restore*/
} nrf_adc_task_t;

/** @brief Analog-to-digital converter events. */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    /*lint -save -e30*/
    NRF_ADC_EVENT_END = offsetof(NRF_ADC_Type, EVENTS_END) /**< End of a conversion event. */
    /*lint -restore*/
} nrf_adc_event_t;

/**@brief Analog-to-digital converter configuration. */
typedef struct
{
    nrf_adc_config_resolution_t resolution; /**< ADC resolution. */
    nrf_adc_config_scaling_t    scaling;    /**< ADC scaling factor. */
    nrf_adc_config_reference_t  reference;  /**< ADC reference. */
    nrf_adc_config_input_t      input;      /**< ADC input selection. */
    nrf_adc_config_extref_t     extref;     /**< ADC external reference selection. */
} nrf_adc_config_t;

/**@brief Analog-to-digital value type. */
typedef uint16_t nrf_adc_value_t;

/**
 * @brief Function for activating a specific ADC task.
 *
 * @param[in] task Task to activate.
 */
__STATIC_INLINE void nrf_adc_task_trigger(nrf_adc_task_t task);

/**
 * @brief Function for getting the address of an ADC task register.
 *
 * @param[in] task ADC task.
 *
 * @return Address of the specified ADC task.
 */
__STATIC_INLINE uint32_t nrf_adc_task_address_get(nrf_adc_task_t task);

/**
 * @brief Function for checking the state of an ADC event.
 *
 * @param[in] event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_adc_event_check(nrf_adc_event_t event);

/**
 * @brief Function for clearing an ADC event.
 *
 * @param[in] event Event to clear.
 */
__STATIC_INLINE void nrf_adc_event_clear(nrf_adc_event_t event);

/**
 * @brief Function for getting the address of a specific ADC event register.
 *
 * @param[in] adc_event ADC event.
 *
 * @return Address of the specified ADC event.
 */
__STATIC_INLINE uint32_t nrf_adc_event_address_get(nrf_adc_event_t adc_event);

/**
 * @brief Function for enabling the specified interrupts.
 *
 * @param[in] int_mask  Interrupts to enable.
 */
__STATIC_INLINE void nrf_adc_int_enable(uint32_t int_mask);

/**
 * @brief Function for disabling the specified interrupts.
 *
 * @param[in] int_mask  Interrupts to disable.
 */
__STATIC_INLINE void nrf_adc_int_disable(uint32_t int_mask);

/**
 * @brief Function for retrieving the state of the specified ADC interrupts.
 *
 * @param[in] int_mask Interrupts to check.
 *
 * @retval true  If all specified interrupts are enabled.
 * @retval false If at least one of the given interrupts is not enabled.
 */
__STATIC_INLINE bool nrf_adc_int_enable_check(uint32_t int_mask);

/**
 * @brief Function for checking whether the ADC is busy.
 *
 * This function checks whether the ADC converter is busy with a conversion.
 *
 * @retval true  If the ADC is busy.
 * @retval false If the ADC is not busy.
 */
__STATIC_INLINE bool nrf_adc_busy_check(void);

/**
 * @brief Function for enabling the ADC.
 *
 */
__STATIC_INLINE void nrf_adc_enable(void);

/**
 * @brief Function for disabling the ADC.
 *
 */
__STATIC_INLINE void nrf_adc_disable(void);

/**
 * @brief Function for checking if the ADC is enabled.
 *
 * @retval true  If the ADC is enabled.
 * @retval false If the ADC is not enabled.
 */
__STATIC_INLINE bool nrf_adc_enable_check(void);

/**
 * @brief Function for retrieving the ADC conversion result.
 *
 * This function retrieves and returns the last analog-to-digital conversion result.
 *
 * @return Last conversion result.
 */
__STATIC_INLINE nrf_adc_value_t nrf_adc_result_get(void);

/**
 * @brief Function for initializing the ADC.
 *
 * This function writes data to ADC's CONFIG register. After the configuration,
 * the ADC is in DISABLE state and must be enabled before using it.
 *
 * @param[in] p_config Configuration parameters.
 */
__STATIC_INLINE void nrf_adc_init(nrf_adc_config_t const * p_config);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_adc_task_trigger(nrf_adc_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_ADC + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_adc_task_address_get(nrf_adc_task_t adc_task)
{
    return (uint32_t)((uint8_t *)NRF_ADC + (uint32_t)adc_task);
}

__STATIC_INLINE bool nrf_adc_event_check(nrf_adc_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)NRF_ADC + (uint32_t)event);
}

__STATIC_INLINE void nrf_adc_event_clear(nrf_adc_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_ADC + (uint32_t)event)) = 0x0UL;
}

__STATIC_INLINE uint32_t nrf_adc_event_address_get(nrf_adc_event_t adc_event)
{
    return (uint32_t)((uint8_t *)NRF_ADC + (uint32_t)adc_event);
}

__STATIC_INLINE void nrf_adc_int_enable(uint32_t int_mask)
{
    NRF_ADC->INTENSET = int_mask;
}

__STATIC_INLINE void nrf_adc_int_disable(uint32_t int_mask)
{
    NRF_ADC->INTENCLR = int_mask;
}

__STATIC_INLINE bool nrf_adc_int_enable_check(uint32_t int_mask)
{
    return (bool)(NRF_ADC->INTENSET & int_mask);
}

__STATIC_INLINE bool nrf_adc_busy_check(void)
{
    return ((NRF_ADC->BUSY & ADC_BUSY_BUSY_Msk) == (ADC_BUSY_BUSY_Busy << ADC_BUSY_BUSY_Pos));
}

__STATIC_INLINE void nrf_adc_enable(void)
{
    NRF_ADC->ENABLE = (ADC_ENABLE_ENABLE_Enabled << ADC_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_adc_disable(void)
{
    NRF_ADC->ENABLE = (ADC_ENABLE_ENABLE_Disabled << ADC_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE bool nrf_adc_enable_check(void)
{
    return (NRF_ADC->ENABLE == (ADC_ENABLE_ENABLE_Enabled << ADC_ENABLE_ENABLE_Pos));
}

__STATIC_INLINE nrf_adc_value_t nrf_adc_result_get(void)
{
    return (nrf_adc_value_t)NRF_ADC->RESULT;
}

__STATIC_INLINE void nrf_adc_init(nrf_adc_config_t const * p_config)
{
    NRF_ADC->CONFIG =
            ((p_config->resolution << ADC_CONFIG_RES_Pos)       & ADC_CONFIG_RES_Msk)
           |((p_config->scaling    << ADC_CONFIG_INPSEL_Pos)    & ADC_CONFIG_INPSEL_Msk)
           |((p_config->reference  << ADC_CONFIG_REFSEL_Pos)    & ADC_CONFIG_REFSEL_Msk)
           |((p_config->input      << ADC_CONFIG_PSEL_Pos)      & ADC_CONFIG_PSEL_Msk)
           |((p_config->extref     << ADC_CONFIG_EXTREFSEL_Pos) & ADC_CONFIG_EXTREFSEL_Msk);
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_ADC_H_ */
