/**
 * Copyright (c) 2015 - 2017, Nordic Semiconductor ASA
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
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_COMP_H__
#define NRFX_COMP_H__

#include <nrfx.h>
#include <hal/nrf_comp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_comp COMP driver
 * @{
 * @ingroup nrf_comp
 * @brief   Comparator (COMP) peripheral driver.
 */

/**
 * @brief Macro to convert the threshold voltage to an integer value
 *        (needed by the COMP_TH register).
 *
 * @param[in] vol   Voltage to be changed to COMP_TH register value. This value
 *                  must not be smaller than reference voltage divided by 64.
 * @param[in] ref   Reference voltage.
 */
#define NRFX_VOLTAGE_THRESHOLD_TO_INT(vol, ref) \
    (uint8_t)(((vol) > ((ref) / 64)) ? (NRFX_ROUNDED_DIV((vol) * 64,(ref)) - 1) : 0)

/**
 * @brief COMP event handler function type.
 * @param[in] event  COMP event.
 */
typedef void (* nrfx_comp_event_handler_t)(nrf_comp_event_t event);

/** @brief COMP shortcut masks. */
typedef enum
{
    NRFX_COMP_SHORT_STOP_AFTER_CROSS_EVT = COMP_SHORTS_CROSS_STOP_Msk, /*!< Shortcut between the CROSS event and the STOP task. */
    NRFX_COMP_SHORT_STOP_AFTER_UP_EVT = COMP_SHORTS_UP_STOP_Msk,       /*!< Shortcut between the UP event and the STOP task. */
    NRFX_COMP_SHORT_STOP_AFTER_DOWN_EVT = COMP_SHORTS_DOWN_STOP_Msk    /*!< Shortcut between the DOWN event and the STOP task. */
} nrfx_comp_short_mask_t;

/** @brief COMP events masks. */
typedef enum
{
    NRFX_COMP_EVT_EN_CROSS_MASK = COMP_INTENSET_CROSS_Msk, /*!< CROSS event (generated after VIN+ == VIN-). */
    NRFX_COMP_EVT_EN_UP_MASK = COMP_INTENSET_UP_Msk,       /*!< UP event (generated when VIN+ crosses VIN- while increasing). */
    NRFX_COMP_EVT_EN_DOWN_MASK = COMP_INTENSET_DOWN_Msk,   /*!< DOWN event (generated when VIN+ crosses VIN- while decreasing). */
    NRFX_COMP_EVT_EN_READY_MASK = COMP_INTENSET_READY_Msk  /*!< READY event (generated when the module is ready). */
} nrfx_comp_evt_en_mask_t;

/** @brief COMP configuration. */
typedef struct
{
    nrf_comp_ref_t          reference;          /**< Reference selection. */
    nrf_comp_ext_ref_t      ext_ref;            /**< External analog reference selection. */
    nrf_comp_main_mode_t    main_mode;          /**< Main operation mode. */
    nrf_comp_th_t           threshold;          /**< Structure holding THDOWN and THUP values needed by the COMP_TH register. */
    nrf_comp_sp_mode_t      speed_mode;         /**< Speed and power mode. */
    nrf_comp_hyst_t         hyst;               /**< Comparator hysteresis.*/
#if defined (COMP_ISOURCE_ISOURCE_Msk) || defined (__NRFX_DOXYGEN__)
    nrf_isource_t           isource;            /**< Current source selected on analog input. */
#endif
    nrf_comp_input_t        input;              /**< Input to be monitored. */
    uint8_t                 interrupt_priority; /**< Interrupt priority. */
} nrfx_comp_config_t;

/** @brief COMP threshold default configuration. */
#define NRFX_COMP_CONFIG_TH                             \
{                                                       \
    .th_down = NRFX_VOLTAGE_THRESHOLD_TO_INT(0.5, 1.8), \
    .th_up   = NRFX_VOLTAGE_THRESHOLD_TO_INT(1.5, 1.8)  \
}

/** @brief COMP driver default configuration including the COMP HAL configuration. */
#if defined (COMP_ISOURCE_ISOURCE_Msk) || defined (__NRFX_DOXYGEN__)
#define NRFX_COMP_DEFAULT_CONFIG(_input)                                    \
{                                                                           \
    .reference          = (nrf_comp_ref_t)NRFX_COMP_CONFIG_REF,             \
    .main_mode          = (nrf_comp_main_mode_t)NRFX_COMP_CONFIG_MAIN_MODE, \
    .threshold          = NRFX_COMP_CONFIG_TH,                              \
    .speed_mode         = (nrf_comp_sp_mode_t)NRFX_COMP_CONFIG_SPEED_MODE,  \
    .hyst               = (nrf_comp_hyst_t)NRFX_COMP_CONFIG_HYST,           \
    .isource            = (nrf_isource_t)NRFX_COMP_CONFIG_ISOURCE,          \
    .input              = (nrf_comp_input_t)_input,                         \
    .interrupt_priority = NRFX_COMP_CONFIG_IRQ_PRIORITY                     \
}
#else
#define NRFX_COMP_DEFAULT_CONFIG(_input)                                    \
{                                                                           \
    .reference          = (nrf_comp_ref_t)NRFX_COMP_CONFIG_REF,             \
    .main_mode          = (nrf_comp_main_mode_t)NRFX_COMP_CONFIG_MAIN_MODE, \
    .threshold          = NRFX_COMP_CONFIG_TH,                              \
    .speed_mode         = (nrf_comp_sp_mode_t)NRFX_COMP_CONFIG_SPEED_MODE,  \
    .hyst               = (nrf_comp_hyst_t)NRFX_COMP_CONFIG_HYST,           \
    .input              = (nrf_comp_input_t)_input,                         \
    .interrupt_priority = NRFX_COMP_CONFIG_IRQ_PRIORITY                     \
}
#endif

/**
 * @brief Function for initializing the COMP driver.
 *
 * This function initializes the COMP driver, but does not enable the peripheral or any interrupts.
 * To start the driver, call the function @ref nrfx_comp_start() after initialization.
 *
 * @param[in] p_config      Pointer to the structure with initial configuration.
 * @param[in] event_handler Handler function.
 *
 * @retval NRFX_ERROR_INVALID_PARAM If the configuration is invalid.
 * @retval NRFX_ERROR_INVALID_STATE If the driver has already been initialized.
 * @retval NRFX_ERROR_BUSY          If the LPCOMP driver is initialized.
 */
nrfx_err_t nrfx_comp_init(nrfx_comp_config_t const * p_config,
                          nrfx_comp_event_handler_t  event_handler);


/**
 *  @brief Function for uninitializing the COMP driver.
 *
 *  This function uninitializes the COMP driver. The COMP peripheral and
 *  its interrupts are disabled, and local variables are cleaned. After this call, you must
 *  initialize the driver again by calling nrfx_comp_init() if you want to use it.
 *
 *  @sa nrfx_comp_stop()
 */
void nrfx_comp_uninit(void);

/**
 * @brief Function for setting the analog input.
 *
 * @param[in] psel  COMP analog pin selection.
 */
void nrfx_comp_pin_select(nrf_comp_input_t psel);

/**
 * @brief Function for starting the COMP peripheral and interrupts.
 *
 * Before calling this function, the driver must be initialized. This function
 * enables the COMP peripheral and its interrupts.
 *
 * @param[in] comp_evt_en_mask  Mask of events to be enabled. This parameter should be built as
 *                              'or' of elements from @ref nrfx_comp_evt_en_mask_t.
 * @param[in] comp_shorts_mask  Mask of shorts to be enabled. This parameter should be built as
 *                              'or' of elements from @ref nrfx_comp_short_mask_t.
 *
 * @sa nrfx_comp_init()
 *
 */
void nrfx_comp_start(uint32_t comp_evt_en_mask, uint32_t comp_shorts_mask);

/**@brief Function for stopping the COMP peripheral.
 *
 * Before calling this function, the driver must be enabled. This function disables the COMP
 * peripheral and its interrupts.
 *
 * @sa nrfx_comp_uninit()
 *
 */
void nrfx_comp_stop(void);

/**
 * @brief Function for copying the current state of the comparator result to the RESULT register.
 *
 * @retval 0 If the input voltage is below the threshold (VIN+ < VIN-).
 * @retval 1 If the input voltage is above the threshold (VIN+ > VIN-).
 */
uint32_t nrfx_comp_sample(void);

/**
 * @brief Function for getting the address of a COMP task.
 *
 * @param[in] task  COMP task.
 *
 * @return Address of the given COMP task.
 */
__STATIC_INLINE uint32_t nrfx_comp_task_address_get(nrf_comp_task_t task)
{
    return (uint32_t)nrf_comp_task_address_get(task);
}

/**
 * @brief Function for getting the address of a COMP event.
 *
 * @param[in] event COMP event.
 *
 * @return Address of the given COMP event.
 */
__STATIC_INLINE uint32_t nrfx_comp_event_address_get(nrf_comp_event_t event)
{
    return (uint32_t)nrf_comp_event_address_get(event);
}


void nrfx_comp_irq_handler(void);


/** @} **/

#ifdef __cplusplus
}
#endif

#endif // NRFX_COMP_H__
