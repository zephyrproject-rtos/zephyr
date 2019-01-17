/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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
 * @brief Front End Module control for the nRF 802.15.4 radio driver.
 *
 */

#ifndef NRF_FEM_CONTROL_API_H_
#define NRF_FEM_CONTROL_API_H_

#include <stdint.h>
#include <stdbool.h>

#include "hal/nrf_ppi.h"
#include "hal/nrf_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @section Resource configuration.
 */

/** Default Power Amplifier pin. */
#define NRF_FEM_CONTROL_DEFAULT_PA_PIN             15

/** Default Low Noise Amplifier pin. */
#define NRF_FEM_CONTROL_DEFAULT_LNA_PIN            16

/** Default PPI channel for pin setting. */
#define NRF_FEM_CONTROL_DEFAULT_SET_PPI_CHANNEL    15

/** Default PPI channel for pin clearing. */
#define NRF_FEM_CONTROL_DEFAULT_CLR_PPI_CHANNEL    16

/** Default GPIOTE channel for FEM control. */
#define NRF_FEM_CONTROL_DEFAULT_LNA_GPIOTE_CHANNEL 6

/** Default GPIOTE channel for FEM control. */
#define NRF_FEM_CONTROL_DEFAULT_PA_GPIOTE_CHANNEL  7

#if ENABLE_FEM

/**
 * @brief Configuration parameters for the PA and LNA.
 */
typedef struct
{
    uint8_t enable      : 1; /**< Enable toggling for this amplifier */
    uint8_t active_high : 1; /**< Set the pin to be active high */
    uint8_t gpio_pin    : 6; /**< The GPIO pin to toggle for this amplifier */
} nrf_fem_control_pa_lna_cfg_t;

/**
 * @brief PA & LNA GPIO toggle configuration
 *
 * This option configures the nRF 802.15.4 radio driver to toggle pins when the radio
 * is active for use with a power amplifier and/or a low noise amplifier.
 *
 * Toggling the pins is achieved by using two PPI channels and a GPIOTE channel. The hardware channel IDs are provided
 * by the application and should be regarded as reserved as long as any PA/LNA toggling is enabled.
 *
 * @note Changing this configuration while the radio is in use may have undefined
 *       consequences and must be avoided by the application.
 */
typedef struct
{
    nrf_fem_control_pa_lna_cfg_t pa_cfg;           /**< Power Amplifier configuration */
    nrf_fem_control_pa_lna_cfg_t lna_cfg;          /**< Low Noise Amplifier configuration */
    uint8_t                      pa_gpiote_ch_id;  /**< GPIOTE channel used for Power Amplifier pin toggling */
    uint8_t                      lna_gpiote_ch_id; /**< GPIOTE channel used for Low Noise Amplifier pin toggling */
    uint8_t                      ppi_ch_id_set;    /**< PPI channel used for radio Power Amplifier and Low Noise Amplifier pins setting */
    uint8_t                      ppi_ch_id_clr;    /**< PPI channel used for radio pin clearing */
} nrf_fem_control_cfg_t;

/**
 * @brief Hardware pins controlled by the Front End Module.
 */
typedef enum
{
    NRF_FEM_CONTROL_PA_PIN,
    NRF_FEM_CONTROL_LNA_PIN,
    NRF_FEM_CONTROL_ANY_PIN,
} nrf_fem_control_pin_t;

/**@brief Set PA & LNA GPIO toggle configuration.
 *
 * @note This function shall not be called when radio is in use.
 *
 * @param[in] p_cfg A pointer to the PA & LNA GPIO toggle configuration.
 *
 */
void nrf_fem_control_cfg_set(const nrf_fem_control_cfg_t * p_cfg);

/**@brief Get PA & LNA GPIO toggle configuration.
 *
 * @param[out] p_cfg A pointer to the structure for the PA & LNA GPIO toggle configuration.
 *
 */
void nrf_fem_control_cfg_get(nrf_fem_control_cfg_t * p_cfg);

/**@brief Activate FEM controller.
 *
 * This function should be called when radio wakes up.
 */
void nrf_fem_control_activate(void);

/**@brief Deactivate FEM controller.
 *
 * This function should be called when radio goes to sleep.
 */
void nrf_fem_control_deactivate(void);

/**@brief Configure PPI to activate one of the Front End Module pins on an appropriate timer event.
 *
 * @param[in] pin              The Front End Module controlled pin to be connected to the PPI.
 * @param[in] timer_cc_channel Timer CC channel that triggers the @p pin activation through the PPI.
 *
 */
void nrf_fem_control_ppi_enable(nrf_fem_control_pin_t pin, nrf_timer_cc_channel_t timer_cc_channel);

/**@brief Clear PPI configuration used to activate one of Front End Module pins.
 *
 * @param[in] pin              The Front End Module controlled pin to be disconnected from the PPI.
 *
 */
void nrf_fem_control_ppi_disable(nrf_fem_control_pin_t pin);

/**@brief Calculate target time for a timer, which activates one of the Front End Module pins.
 *
 * @param[in] pin The Front End Module controlled pin to be activated.
 *
 * @return    @p pin activation delay in microseconds.
 *
 */
uint32_t nrf_fem_control_delay_get(nrf_fem_control_pin_t pin);

/**@brief Clear the Power Amplifier and the Low Noise Amplifier pins immediately.
 *
 */
void nrf_fem_control_pin_clear(void);

/**@brief Configure and set a timer for one of the Front End Module pins activation.
 *
 * @param[in] pin              The Front End Module controlled pin to be activated.
 * @param[in] timer_cc_channel Timer CC channel to set.
 * @param[in] short_mask       Mask of timer shortcuts to be enabled.
 *
 */
void nrf_fem_control_timer_set(nrf_fem_control_pin_t  pin,
                               nrf_timer_cc_channel_t timer_cc_channel,
                               nrf_timer_short_mask_t short_mask);

/**@brief Clear timer configuration after one of the Front End Module pins deactivation.
 *
 * @param[in] pin              The Front End Module controlled pin that was deactivated.
 * @param[in] short_mask       Mask of timer shortcuts to be disabled.
 *
 */
void nrf_fem_control_timer_reset(nrf_fem_control_pin_t pin, nrf_timer_short_mask_t short_mask);

/**@brief Setup a PPI fork task necessary for one of the Front End Module pins.
 *
 * @param[in] pin              The Front End Module controlled pin that was deactivated.
 * @param[in] ppi_channel      PPI channel to connect the fork task to.
 *
 */
void nrf_fem_control_ppi_fork_setup(nrf_fem_control_pin_t pin,
                                    nrf_ppi_channel_t     ppi_channel,
                                    uint32_t              task_addr);

/**@brief Setup a PPI task necessary for one of the Front End Module pins.
 *
 * @param[in] pin              The Front End Module controlled pin that was deactivated.
 * @param[in] ppi_channel      PPI channel to connect the task to.
 * @param[in] event_addr       Address of the event to be connected to the PPI.
 * @param[in] task_addr        Address of the task to be connected to the PPI.
 *
 */
void nrf_fem_control_ppi_task_setup(nrf_fem_control_pin_t pin,
                                    nrf_ppi_channel_t     ppi_channel,
                                    uint32_t              event_addr,
                                    uint32_t              task_addr);

/**@brief Clear a PPI fork task configuration for one of the Front End Module pins.
 *
 * @param[in] pin              The Front End Module controlled pin that was deactivated.
 * @param[in] ppi_channel      PPI channel to disconnect the fork task from.
 *
 */
void nrf_fem_control_ppi_fork_clear(nrf_fem_control_pin_t pin, nrf_ppi_channel_t ppi_channel);

/**@brief Setup PPI task and fork that set or clear Front End Module pins on a given event.
 *
 * @param[in] ppi_channel      PPI channel to connect the task and fork to.
 * @param[in] event_addr       Address of the event to be connected to the PPI.
 * @param[in] lna_pin_set      If true, the Low Noise Amplifier pin will be set on the event @p event_addr.
 *                             Otherwise, it will be cleared.
 * @param[in] pa_pin_set       If true, the Power Amplifier pin will be set on the event @p event_addr.
 *                             Otherwise, it will be cleared.
 *
 */
void nrf_fem_control_ppi_pin_task_setup(nrf_ppi_channel_t ppi_channel,
                                        uint32_t          event_addr,
                                        bool              lna_pin_set,
                                        bool              pa_pin_set);

#else // ENABLE_FEM

#define nrf_fem_control_cfg_set(...)
#define nrf_fem_control_cfg_get(...)
#define nrf_fem_control_activate(...)
#define nrf_fem_control_deactivate(...)
#define nrf_fem_control_ppi_enable(...)
#define nrf_fem_control_ppi_disable(...)
#define nrf_fem_control_delay_get(...) 1
#define nrf_fem_control_pin_clear(...)
#define nrf_fem_control_timer_set(...)
#define nrf_fem_control_timer_reset(...)
#define nrf_fem_control_ppi_fork_setup(...)
#define nrf_fem_control_ppi_task_setup(...)
#define nrf_fem_control_ppi_task_and_fork_setup(...)
#define nrf_fem_control_ppi_fork_clear(...)
#define nrf_fem_control_ppi_pin_task_setup(...)

#endif // ENABLE_FEM

#ifdef __cplusplus
}
#endif

#endif /* NRF_FEM_CONTROL_API_H_ */
