/*
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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

#include <nrfx.h>

#if NRFX_CHECK(NRFX_POWER_ENABLED)

#include <nrfx_power.h>

#if NRFX_CHECK(NRFX_CLOCK_ENABLED)
extern bool nrfx_clock_irq_enabled;
#endif

/**
 * @internal
 * @defgroup nrfx_power_internals POWER driver internals
 * @ingroup nrfx_power
 *
 * Internal variables, auxiliary macros and functions of POWER driver.
 * @{
 */

/**
 * This variable is used to check whether common POWER_CLOCK common interrupt
 * should be disabled or not if @ref nrfx_clock tries to disable the interrupt.
 */

bool nrfx_power_irq_enabled;

/**
 * @brief The initialization flag
 */

#define m_initialized nrfx_power_irq_enabled

/**
 * @brief The handler of power fail comparator warning event
 */
static nrfx_power_pofwarn_event_handler_t m_pofwarn_handler;

#if NRF_POWER_HAS_SLEEPEVT || defined(__NRFX_DOXYGEN__)
/**
 * @brief The handler of sleep event handler
 */
static nrfx_power_sleep_event_handler_t m_sleepevt_handler;
#endif

#if NRF_POWER_HAS_USBREG || defined(__NRFX_DOXYGEN__)
/**
 * @brief The handler of USB power events
 */
static nrfx_power_usb_event_handler_t m_usbevt_handler;
#endif

/** @} */

nrfx_power_pofwarn_event_handler_t nrfx_power_pof_handler_get(void)
{
    return m_pofwarn_handler;
}

#if NRF_POWER_HAS_USBREG
nrfx_power_usb_event_handler_t nrfx_power_usb_handler_get(void)
{
    return m_usbevt_handler;
}
#endif

nrfx_err_t nrfx_power_init(nrfx_power_config_t const * p_config)
{
    NRFX_ASSERT(p_config);
    if (m_initialized)
    {
        return NRFX_ERROR_ALREADY_INITIALIZED;
    }

#if NRF_POWER_HAS_VDDH
    nrf_power_dcdcen_vddh_set(p_config->dcdcenhv);
#endif
    nrf_power_dcdcen_set(p_config->dcdcen);

    nrfx_power_clock_irq_init();

    m_initialized = true;
    return NRFX_SUCCESS;
}


void nrfx_power_uninit(void)
{
    NRFX_ASSERT(m_initialized);

#if NRFX_CHECK(NRFX_CLOCK_ENABLED)
    if (!nrfx_clock_irq_enabled)
#endif
    {
        NRFX_IRQ_DISABLE(POWER_CLOCK_IRQn);
    }

    nrfx_power_pof_uninit();
#if NRF_POWER_HAS_SLEEPEVT || defined(__NRFX_DOXYGEN__)
    nrfx_power_sleepevt_uninit();
#endif
#if NRF_POWER_HAS_USBREG || defined(__NRFX_DOXYGEN__)
    nrfx_power_usbevt_uninit();
#endif
    m_initialized = false;
}

void nrfx_power_pof_init(nrfx_power_pofwarn_config_t const * p_config)
{
    NRFX_ASSERT(p_config != NULL);

    nrfx_power_pof_uninit();

    if (p_config->handler != NULL)
    {
        m_pofwarn_handler = p_config->handler;
    }
}

void nrfx_power_pof_enable(nrfx_power_pofwarn_config_t const * p_config)
{
    nrf_power_pofcon_set(true, p_config->thr);
#if NRF_POWER_HAS_VDDH || defined(__NRFX_DOXYGEN__)
    nrf_power_pofcon_vddh_set(p_config->thrvddh);
#endif
    if (m_pofwarn_handler != NULL)
    {
        nrf_power_int_enable(NRF_POWER_INT_POFWARN_MASK);
    }
}

void nrfx_power_pof_disable(void)
{
    nrf_power_int_disable(NRF_POWER_INT_POFWARN_MASK);
}

void nrfx_power_pof_uninit(void)
{
    m_pofwarn_handler = NULL;
}

#if NRF_POWER_HAS_SLEEPEVT || defined(__NRFX_DOXYGEN__)
void nrfx_power_sleepevt_init(nrfx_power_sleepevt_config_t const * p_config)
{
    NRFX_ASSERT(p_config != NULL);

    nrfx_power_sleepevt_uninit();
    if (p_config->handler != NULL)
    {
        m_sleepevt_handler = p_config->handler;
    }
}

void nrfx_power_sleepevt_enable(nrfx_power_sleepevt_config_t const * p_config)
{
    uint32_t enmask = 0;
    if (p_config->en_enter)
    {
        enmask |= NRF_POWER_INT_SLEEPENTER_MASK;
        nrf_power_event_clear(NRF_POWER_EVENT_SLEEPENTER);
    }
    if (p_config->en_exit)
    {
        enmask |= NRF_POWER_INT_SLEEPEXIT_MASK;
        nrf_power_event_clear(NRF_POWER_EVENT_SLEEPEXIT);
    }
    nrf_power_int_enable(enmask);
}

void nrfx_power_sleepevt_disable(void)
{
    nrf_power_int_disable(
        NRF_POWER_INT_SLEEPENTER_MASK |
        NRF_POWER_INT_SLEEPEXIT_MASK);
}

void nrfx_power_sleepevt_uninit(void)
{
    m_sleepevt_handler = NULL;
}
#endif /* NRF_POWER_HAS_SLEEPEVT */

#if NRF_POWER_HAS_USBREG || defined(__NRFX_DOXYGEN__)
void nrfx_power_usbevt_init(nrfx_power_usbevt_config_t const * p_config)
{
    nrfx_power_usbevt_uninit();
    if (p_config->handler != NULL)
    {
        m_usbevt_handler = p_config->handler;
    }
}

void nrfx_power_usbevt_enable(void)
{
    nrf_power_int_enable(
        NRF_POWER_INT_USBDETECTED_MASK |
        NRF_POWER_INT_USBREMOVED_MASK  |
        NRF_POWER_INT_USBPWRRDY_MASK);
}

void nrfx_power_usbevt_disable(void)
{
    nrf_power_int_disable(
        NRF_POWER_INT_USBDETECTED_MASK |
        NRF_POWER_INT_USBREMOVED_MASK  |
        NRF_POWER_INT_USBPWRRDY_MASK);
}

void nrfx_power_usbevt_uninit(void)
{
    m_usbevt_handler = NULL;
}
#endif /* NRF_POWER_HAS_USBREG */


void nrfx_power_irq_handler(void)
{
    uint32_t enabled = nrf_power_int_enable_get();
    if ((0 != (enabled & NRF_POWER_INT_POFWARN_MASK)) &&
        nrf_power_event_get_and_clear(NRF_POWER_EVENT_POFWARN))
    {
        /* Cannot be null if event is enabled */
        NRFX_ASSERT(m_pofwarn_handler != NULL);
        m_pofwarn_handler();
    }
#if NRF_POWER_HAS_SLEEPEVT || defined(__NRFX_DOXYGEN__)
    if ((0 != (enabled & NRF_POWER_INT_SLEEPENTER_MASK)) &&
        nrf_power_event_get_and_clear(NRF_POWER_EVENT_SLEEPENTER))
    {
        /* Cannot be null if event is enabled */
        NRFX_ASSERT(m_sleepevt_handler != NULL);
        m_sleepevt_handler(NRFX_POWER_SLEEP_EVT_ENTER);
    }
    if ((0 != (enabled & NRF_POWER_INT_SLEEPEXIT_MASK)) &&
        nrf_power_event_get_and_clear(NRF_POWER_EVENT_SLEEPEXIT))
    {
        /* Cannot be null if event is enabled */
        NRFX_ASSERT(m_sleepevt_handler != NULL);
        m_sleepevt_handler(NRFX_POWER_SLEEP_EVT_EXIT);
    }
#endif
#if NRF_POWER_HAS_USBREG || defined(__NRFX_DOXYGEN__)
    if ((0 != (enabled & NRF_POWER_INT_USBDETECTED_MASK)) &&
        nrf_power_event_get_and_clear(NRF_POWER_EVENT_USBDETECTED))
    {
        /* Cannot be null if event is enabled */
        NRFX_ASSERT(m_usbevt_handler != NULL);
        m_usbevt_handler(NRFX_POWER_USB_EVT_DETECTED);
    }
    if ((0 != (enabled & NRF_POWER_INT_USBREMOVED_MASK)) &&
        nrf_power_event_get_and_clear(NRF_POWER_EVENT_USBREMOVED))
    {
        /* Cannot be null if event is enabled */
        NRFX_ASSERT(m_usbevt_handler != NULL);
        m_usbevt_handler(NRFX_POWER_USB_EVT_REMOVED);
    }
    if ((0 != (enabled & NRF_POWER_INT_USBPWRRDY_MASK)) &&
        nrf_power_event_get_and_clear(NRF_POWER_EVENT_USBPWRRDY))
    {
        /* Cannot be null if event is enabled */
        NRFX_ASSERT(m_usbevt_handler != NULL);
        m_usbevt_handler(NRFX_POWER_USB_EVT_READY);
    }
#endif
}

#endif // NRFX_CHECK(NRFX_POWER_ENABLED)
