/*
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
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

#ifndef NRFX_WDT_H__
#define NRFX_WDT_H__

#include <nrfx.h>
#include <hal/nrf_wdt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_wdt WDT driver
 * @{
 * @ingroup nrf_wdt
 * @brief   Watchdog Timer (WDT) peripheral driver.
 */

#if !NRFX_CHECK(NRFX_WDT_CONFIG_NO_IRQ) || defined(__NRFX_DOXYGEN__)
/** @brief WDT instance interrupt priority configuration. */
    #define NRFX_WDT_IRQ_CONFIG .interrupt_priority = NRFX_WDT_CONFIG_IRQ_PRIORITY
#else
    #define NRFX_WDT_IRQ_CONFIG
#endif

/**@brief Struct for WDT initialization. */
typedef struct
{
    nrf_wdt_behaviour_t    behaviour;          /**< WDT behaviour when CPU in sleep/halt mode. */
    uint32_t               reload_value;       /**< WDT reload value in ms. */
#if !NRFX_CHECK(NRFX_WDT_CONFIG_NO_IRQ) || defined(__NRFX_DOXYGEN__)
    uint8_t                interrupt_priority; /**< WDT interrupt priority */
#endif
} nrfx_wdt_config_t;

/** @brief WDT event handler function type. */
typedef void (*nrfx_wdt_event_handler_t)(void);

/** @brief WDT channel ID type. */
typedef nrf_wdt_rr_register_t nrfx_wdt_channel_id;

/** @brief WDT driver default configuration. */
#define NRFX_WDT_DEAFULT_CONFIG                                               \
    {                                                                         \
        .behaviour          = (nrf_wdt_behaviour_t)NRFX_WDT_CONFIG_BEHAVIOUR, \
        .reload_value       = NRFX_WDT_CONFIG_RELOAD_VALUE,                   \
        NRFX_WDT_IRQ_CONFIG                                                   \
    }
/**
 * @brief This function initializes the watchdog.
 *
 * @param[in] p_config          Pointer to the structure with the initial configuration.
 * @param[in] wdt_event_handler Event handler provided by the user. Ignored when
 *                              @ref NRFX_WDT_CONFIG_NO_IRQ option is enabled.
 *
 * @return NRFX_SUCCESS on success, otherwise an error code.
 */
nrfx_err_t nrfx_wdt_init(nrfx_wdt_config_t const * p_config,
                         nrfx_wdt_event_handler_t  wdt_event_handler);

/**
 * @brief Function for allocating a watchdog channel.
 *
 * @note This function can not be called after nrfx_wdt_start(void).
 *
 * @param[out] p_channel_id ID of granted channel.
 *
 * @return NRFX_SUCCESS on success, otherwise an error code.
 */
nrfx_err_t nrfx_wdt_channel_alloc(nrfx_wdt_channel_id * p_channel_id);

/**
 * @brief Function for starting the watchdog.
 *
 * @note After calling this function the watchdog is started, so the user needs to feed all allocated
 *       watchdog channels to avoid reset. At least one watchdog channel must be allocated.
 */
void nrfx_wdt_enable(void);

/**
 * @brief Function for feeding the watchdog.
 *
 * @details Function feeds all allocated watchdog channels.
 */
void nrfx_wdt_feed(void);

/**
 * @brief Function for feeding an invidual watchdog channel.
 *
 * @param[in] channel_id ID of watchdog channel.
 */
void nrfx_wdt_channel_feed(nrfx_wdt_channel_id channel_id);

/**
 * @brief Function for returning a requested task address for the WDT driver module.
 *
 * @param[in] task One of the peripheral tasks.
 *
 * @return Task address.
 */
__STATIC_INLINE uint32_t nrfx_wdt_ppi_task_addr(nrf_wdt_task_t task)
{
    return nrf_wdt_task_address_get(task);
}

/**
 * @brief Function for returning a requested event address for the wdt driver module.
 *
 * @param[in] event One of the peripheral events.
 *
 * @return Event address.
 */
__STATIC_INLINE uint32_t nrfx_wdt_ppi_event_addr(nrf_wdt_event_t event)
{
    return nrf_wdt_event_address_get(event);
}

/** @} */


void nrfx_wdt_irq_handler(void);


#ifdef __cplusplus
}
#endif

#endif

