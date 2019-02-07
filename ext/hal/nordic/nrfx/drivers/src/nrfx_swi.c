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

#include <nrfx.h>

#if NRFX_CHECK(NRFX_SWI_ENABLED)

#include <nrfx_swi.h>

#define NRFX_LOG_MODULE SWI
#include <nrfx_log.h>


// NRFX_SWI_RESERVED_MASK - SWIs reserved for use by external modules.
#if NRFX_CHECK(NRFX_PWM_NRF52_ANOMALY_109_WORKAROUND_ENABLED)
#define NRFX_SWI_RESERVED_MASK  ((NRFX_SWI_USED) | \
                                 (1u << NRFX_PWM_NRF52_ANOMALY_109_EGU_INSTANCE))
#else
#define NRFX_SWI_RESERVED_MASK  (NRFX_SWI_USED)
#endif

// NRFX_SWI_DISABLED_MASK - SWIs excluded from use in <nrfx_config.h>.
#if NRFX_CHECK(NRFX_SWI0_DISABLED)
#define NRFX_SWI0_DISABLED_MASK (1u << 0)
#else
#define NRFX_SWI0_DISABLED_MASK 0u
#endif
#if NRFX_CHECK(NRFX_SWI1_DISABLED)
#define NRFX_SWI1_DISABLED_MASK (1u << 1)
#else
#define NRFX_SWI1_DISABLED_MASK 0u
#endif
#if NRFX_CHECK(NRFX_SWI2_DISABLED)
#define NRFX_SWI2_DISABLED_MASK (1u << 2)
#else
#define NRFX_SWI2_DISABLED_MASK 0u
#endif
#if NRFX_CHECK(NRFX_SWI3_DISABLED)
#define NRFX_SWI3_DISABLED_MASK (1u << 3)
#else
#define NRFX_SWI3_DISABLED_MASK 0u
#endif
#if NRFX_CHECK(NRFX_SWI4_DISABLED)
#define NRFX_SWI4_DISABLED_MASK (1u << 4)
#else
#define NRFX_SWI4_DISABLED_MASK 0u
#endif
#if NRFX_CHECK(NRFX_SWI5_DISABLED)
#define NRFX_SWI5_DISABLED_MASK (1u << 5)
#else
#define NRFX_SWI5_DISABLED_MASK 0u
#endif
#define NRFX_SWI_DISABLED_MASK  (NRFX_SWI0_DISABLED_MASK | \
                                 NRFX_SWI1_DISABLED_MASK | \
                                 NRFX_SWI2_DISABLED_MASK | \
                                 NRFX_SWI3_DISABLED_MASK | \
                                 NRFX_SWI4_DISABLED_MASK | \
                                 NRFX_SWI5_DISABLED_MASK)

#if (NRFX_SWI_RESERVED_MASK & NRFX_SWI_DISABLED_MASK)
#error "A reserved SWI configured to be disabled. Check <nrfx_config.h> and NRFX_SWI_USED."
#endif

// NRFX_SWI_AVAILABLE_MASK - SWIs available for this module, i.e. present
// in the hardware and neither reserved by external modules nor disabled
// in <nrfx_config.h>.
#define NRFX_SWI_PRESENT_MASK   ((1u << (SWI_COUNT)) - 1u)
#define NRFX_SWI_AVAILABLE_MASK (NRFX_SWI_PRESENT_MASK &    \
                                 ~(NRFX_SWI_RESERVED_MASK | \
                                   NRFX_SWI_DISABLED_MASK))

#if (NRFX_SWI_AVAILABLE_MASK == 0)
#error "No available SWI instances. Check <nrfx_config.h> and NRFX_SWI_USED."
#endif

#define NRFX_SWI_IS_AVAILABLE(idx)  ((NRFX_SWI_AVAILABLE_MASK >> (idx)) & 1u)

#define NRFX_SWI_FIRST  (NRFX_SWI_IS_AVAILABLE(0) ? 0u : \
                        (NRFX_SWI_IS_AVAILABLE(1) ? 1u : \
                        (NRFX_SWI_IS_AVAILABLE(2) ? 2u : \
                        (NRFX_SWI_IS_AVAILABLE(3) ? 3u : \
                        (NRFX_SWI_IS_AVAILABLE(4) ? 4u : \
                                                    5u)))))
#define NRFX_SWI_LAST   (NRFX_SWI_IS_AVAILABLE(5) ? 5u : \
                        (NRFX_SWI_IS_AVAILABLE(4) ? 4u : \
                        (NRFX_SWI_IS_AVAILABLE(3) ? 3u : \
                        (NRFX_SWI_IS_AVAILABLE(2) ? 2u : \
                        (NRFX_SWI_IS_AVAILABLE(1) ? 1u : \
                                                    0u)))))

// NRFX_SWI_EGU_COUNT - number of EGU instances to be used by this module
// (note - if EGU is not present, EGU_COUNT is not defined).
#if NRFX_CHECK(NRFX_EGU_ENABLED)
#define NRFX_SWI_EGU_COUNT  EGU_COUNT
#else
#define NRFX_SWI_EGU_COUNT  0
#endif

// These flags are needed only for SWIs that have no corresponding EGU unit
// (in EGU such flags are available in hardware).
#if (NRFX_SWI_EGU_COUNT < SWI_COUNT)
static nrfx_swi_flags_t   m_swi_flags[SWI_COUNT - NRFX_SWI_EGU_COUNT];
#endif
static nrfx_swi_handler_t m_swi_handlers[SWI_COUNT];
static uint8_t            m_swi_allocated_mask;


static void swi_mark_allocated(nrfx_swi_t swi)
{
    m_swi_allocated_mask |= (1u << swi);
}

static void swi_mark_unallocated(nrfx_swi_t swi)
{
    m_swi_allocated_mask &= ~(1u << swi);
}

static bool swi_is_allocated(nrfx_swi_t swi)
{
    return (m_swi_allocated_mask & (1u << swi));
}

static bool swi_is_available(nrfx_swi_t swi)
{
    return NRFX_SWI_IS_AVAILABLE(swi);
}

static IRQn_Type swi_irq_number_get(nrfx_swi_t swi)
{
#if defined(NRF_SWI)
    return (IRQn_Type)(nrfx_get_irq_number(NRF_SWI) + swi);
#elif defined(NRF_SWI0)
    return (IRQn_Type)(nrfx_get_irq_number(NRF_SWI0) + swi);
#else
    return (IRQn_Type)(nrfx_get_irq_number(NRF_EGU0) + swi);
#endif
}

static void swi_int_enable(nrfx_swi_t swi)
{
#if NRFX_SWI_EGU_COUNT
    if (swi < NRFX_SWI_EGU_COUNT)
    {
        NRF_EGU_Type * p_egu = nrfx_swi_egu_instance_get(swi);
        NRFX_ASSERT(p_egu != NULL);
        nrf_egu_int_enable(p_egu, NRF_EGU_INT_ALL);

        if (m_swi_handlers[swi] == NULL)
        {
            return;
        }
    }
#endif

    NRFX_IRQ_ENABLE(swi_irq_number_get(swi));
}

static void swi_int_disable(nrfx_swi_t swi)
{
    NRFX_IRQ_DISABLE(swi_irq_number_get(swi));

#if NRFX_SWI_EGU_COUNT
    if (swi < NRFX_SWI_EGU_COUNT)
    {
        nrf_egu_int_disable(nrfx_swi_egu_instance_get(swi), NRF_EGU_INT_ALL);
    }
#endif
}

static void swi_handler_setup(nrfx_swi_t         swi,
                              nrfx_swi_handler_t event_handler,
                              uint32_t           irq_priority)
{
    m_swi_handlers[swi] = event_handler;
    NRFX_IRQ_PRIORITY_SET(swi_irq_number_get(swi), irq_priority);
    swi_int_enable(swi);
}

static void swi_deallocate(nrfx_swi_t swi)
{
    swi_int_disable(swi);
    m_swi_handlers[swi] = NULL;
    swi_mark_unallocated(swi);
}

nrfx_err_t nrfx_swi_alloc(nrfx_swi_t *       p_swi,
                          nrfx_swi_handler_t event_handler,
                          uint32_t           irq_priority)
{
    NRFX_ASSERT(p_swi != NULL);

    nrfx_err_t err_code;

    for (nrfx_swi_t swi = NRFX_SWI_FIRST; swi <= NRFX_SWI_LAST; ++swi)
    {
        if (swi_is_available(swi))
        {
            bool allocated = false;
            NRFX_CRITICAL_SECTION_ENTER();
            if (!swi_is_allocated(swi))
            {
                swi_mark_allocated(swi);
                allocated = true;
            }
            NRFX_CRITICAL_SECTION_EXIT();

            if (allocated)
            {
                swi_handler_setup(swi, event_handler, irq_priority);

                *p_swi = swi;
                NRFX_LOG_INFO("SWI channel allocated: %d.", (*p_swi));
                return NRFX_SUCCESS;
            }
        }
    }

    err_code = NRFX_ERROR_NO_MEM;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

bool nrfx_swi_is_allocated(nrfx_swi_t swi)
{
    return swi_is_allocated(swi);
}

void nrfx_swi_int_disable(nrfx_swi_t swi)
{
    NRFX_ASSERT(swi_is_allocated(swi));
    swi_int_disable(swi);
}

void nrfx_swi_int_enable(nrfx_swi_t swi)
{
    NRFX_ASSERT(swi_is_allocated(swi));
    swi_int_enable(swi);
}

void nrfx_swi_free(nrfx_swi_t * p_swi)
{
    NRFX_ASSERT(p_swi != NULL);
    nrfx_swi_t swi = *p_swi;

    NRFX_ASSERT(swi_is_allocated(swi));
    swi_deallocate(swi);

    *p_swi = NRFX_SWI_UNALLOCATED;
}

void nrfx_swi_all_free(void)
{
    for (nrfx_swi_t swi = NRFX_SWI_FIRST; swi <= NRFX_SWI_LAST; ++swi)
    {
        if (swi_is_allocated(swi))
        {
            swi_deallocate(swi);
        }
    }
}

void nrfx_swi_trigger(nrfx_swi_t swi, uint8_t flag_number)
{
    NRFX_ASSERT(swi_is_allocated(swi));

#if NRFX_SWI_EGU_COUNT

    NRF_EGU_Type * p_egu = nrfx_swi_egu_instance_get(swi);
#if (NRFX_SWI_EGU_COUNT < SWI_COUNT)
    if (p_egu == NULL)
    {
        m_swi_flags[swi - NRFX_SWI_EGU_COUNT] |= (1 << flag_number);
        NRFX_IRQ_PENDING_SET(swi_irq_number_get(swi));
    }
    else
#endif // (NRFX_SWI_EGU_COUNT < SWI_COUNT)
    {
        nrf_egu_task_trigger(p_egu,
            nrf_egu_task_trigger_get(p_egu, flag_number));
    }

#else // -> #if !NRFX_SWI_EGU_COUNT

    m_swi_flags[swi - NRFX_SWI_EGU_COUNT] |= (1 << flag_number);
    NRFX_IRQ_PENDING_SET(swi_irq_number_get(swi));

#endif
}

#if NRFX_SWI_EGU_COUNT
static void egu_irq_handler(nrfx_swi_t swi, uint8_t egu_channel_count)
{
#if (NRFX_SWI_FIRST > 0)
    NRFX_ASSERT(swi >= NRFX_SWI_FIRST);
#endif
    NRFX_ASSERT(swi <= NRFX_SWI_LAST);
    nrfx_swi_handler_t handler = m_swi_handlers[swi];
    NRFX_ASSERT(handler != NULL);

    NRF_EGU_Type * p_egu = nrfx_swi_egu_instance_get(swi);
    NRFX_ASSERT(p_egu != NULL);

    nrfx_swi_flags_t flags = 0;
    for (uint8_t i = 0; i < egu_channel_count; ++i)
    {
        nrf_egu_event_t egu_event = nrf_egu_event_triggered_get(p_egu, i);
        if (nrf_egu_event_check(p_egu, egu_event))
        {
            flags |= (1u << i);
            nrf_egu_event_clear(p_egu, egu_event);
        }
    }

    handler(swi, flags);
}
#endif // NRFX_SWI_EGU_COUNT

#if (NRFX_SWI_EGU_COUNT < SWI_COUNT)
static void swi_irq_handler(nrfx_swi_t swi)
{
#if (NRFX_SWI_FIRST > 0)
    NRFX_ASSERT(swi >= NRFX_SWI_FIRST);
#endif
    NRFX_ASSERT(swi <= NRFX_SWI_LAST);
    nrfx_swi_handler_t handler = m_swi_handlers[swi];
    NRFX_ASSERT(handler != NULL);

    nrfx_swi_flags_t flags = m_swi_flags[swi - NRFX_SWI_EGU_COUNT];
    m_swi_flags[swi - NRFX_SWI_EGU_COUNT] &= ~flags;

    handler(swi, flags);
}
#endif // (NRFX_SWI_EGU_COUNT < SWI_COUNT)


#if NRFX_SWI_IS_AVAILABLE(0)
void nrfx_swi_0_irq_handler(void)
{
#if (NRFX_SWI_EGU_COUNT > 0)
    egu_irq_handler(0, EGU0_CH_NUM);
#else
    swi_irq_handler(0);
#endif
}
#endif // NRFX_SWI_IS_AVAILABLE(0)

#if NRFX_SWI_IS_AVAILABLE(1)
void nrfx_swi_1_irq_handler(void)
{
#if (NRFX_SWI_EGU_COUNT > 1)
    egu_irq_handler(1, EGU1_CH_NUM);
#else
    swi_irq_handler(1);
#endif
}
#endif // NRFX_SWI_IS_AVAILABLE(1)

#if  NRFX_SWI_IS_AVAILABLE(2)
void nrfx_swi_2_irq_handler(void)
{
#if (NRFX_SWI_EGU_COUNT > 2)
    egu_irq_handler(2, EGU2_CH_NUM);
#else
    swi_irq_handler(2);
#endif
}
#endif // NRFX_SWI_IS_AVAILABLE(2)

#if NRFX_SWI_IS_AVAILABLE(3)
void nrfx_swi_3_irq_handler(void)
{
#if (NRFX_SWI_EGU_COUNT > 3)
    egu_irq_handler(3, EGU3_CH_NUM);
#else
    swi_irq_handler(3);
#endif
}
#endif // NRFX_SWI_IS_AVAILABLE(3)

#if NRFX_SWI_IS_AVAILABLE(4)
void nrfx_swi_4_irq_handler(void)
{
#if (NRFX_SWI_EGU_COUNT > 4)
    egu_irq_handler(4, EGU4_CH_NUM);
#else
    swi_irq_handler(4);
#endif
}
#endif // NRFX_SWI_IS_AVAILABLE(4)

#if NRFX_SWI_IS_AVAILABLE(5)
void nrfx_swi_5_irq_handler(void)
{
#if (NRFX_SWI_EGU_COUNT > 5)
    egu_irq_handler(5, EGU5_CH_NUM);
#else
    swi_irq_handler(5);
#endif
}
#endif // NRFX_SWI_IS_AVAILABLE(5)

#endif // NRFX_CHECK(NRFX_SWI_ENABLED)
