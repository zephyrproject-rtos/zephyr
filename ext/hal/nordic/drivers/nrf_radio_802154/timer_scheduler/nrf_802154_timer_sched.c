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
 * @file
 *   This file implements timer scheduler for the nRF 802.15.4 driver.
 *
 *  This implementation supports scheduling of multiple timer instances and can be used from different contexts.
 *
 *  @note Timer scheduler is secured against preemption and adding/removing different timers from different contexts,
 *        it shall not be used for adding/removing the same timer instance from two contexts at the same time.
 *
 */

#include "nrf_802154_timer_sched.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <nrf.h>
#include "nrf_802154_debug.h"
#include "platform/lp_timer/nrf_802154_lp_timer.h"

#if defined(__ICCARM__)
_Pragma("diag_suppress=Pe167")
#endif

static volatile uint8_t              m_timer_mutex;        ///< Mutex for starting the timer.
static volatile uint8_t              m_fired_mutex;        ///< Mutex for the timer firing procedure.
static volatile uint8_t              m_queue_changed_cntr; ///< Information that scheduler queue was modified.
static volatile nrf_802154_timer_t * mp_head;              ///< Head of the running timers list.

/** @brief Non-blocking mutex for starting the timer.
 *
 *  @retval  true   Mutex was acquired.
 *  @retval  false  Mutex could not be acquired.
 */
static inline bool mutex_trylock(volatile uint8_t * p_mutex)
{
    do
    {
        volatile uint8_t mutex_value = __LDREXB(p_mutex);

        if (mutex_value)
        {
            __CLREX();
            return false;
        }
    }
    while (__STREXB(1, p_mutex));

    __DMB();

    return true;
}

/** @brief Release mutex. */
static inline void mutex_unlock(volatile uint8_t * p_mutex)
{
    __DMB();
    *p_mutex = 0;
}

/** @brief Increment queue counter value to detect changes in the queue. */
static inline void queue_cntr_bump(void)
{
    volatile uint8_t cntr;

    do
    {
        cntr = __LDREXB(&m_queue_changed_cntr);
    }
    while (__STREXB(cntr + 1, &m_queue_changed_cntr));

    __DMB();
}

/**
 * @brief Check if @p time_1 is before @p time_2.
 *
 * @param[in]  time_1  First time to compare.
 * @param[in]  time_2  Second time to compare.
 *
 * @return  True if @p time_1 is before @p time_2, false otherwise.
 */
static inline bool is_time_before(uint32_t time_1, uint32_t time_2)
{
    int32_t diff = time_1 - time_2;

    return diff < 0;
}

/**
 * @brief Check if @p p_timer_1 shall strike earlier than @p p_timer_2.
 *
 * @param[in]  p_timer_1  A pointer to first timer to compare.
 * @param[in]  p_timer_2  A pointer to second timer to compare.
 *
 * @return  True if @p p_timer_1 shall strike earlier than @p p_timer_2, false otherwise.
 */
static inline bool is_timer_prior(const nrf_802154_timer_t * p_timer_1,
                                  const nrf_802154_timer_t * p_timer_2)
{
    return is_time_before(p_timer_1->t0 + p_timer_1->dt, p_timer_2->t0 + p_timer_2->dt);
}

/**
 * @brief Handle operation on timer with mutex protection.
 */
static inline void handle_timer(void)
{
    volatile nrf_802154_timer_t * p_head;
    uint8_t                       queue_cntr;

    do
    {
        queue_cntr = m_queue_changed_cntr;
        p_head     = mp_head;

        if (mutex_trylock(&m_timer_mutex))
        {
            if (p_head == NULL)
            {
                nrf_802154_lp_timer_stop();
            }
            else
            {
                uint32_t t0 = p_head->t0;
                uint32_t dt = p_head->dt;

                // Set the timer only if current HEAD wasn't removed - otherwise t0 and dt might've been modified
                // between reading t0 and dt and not be a valid combination.
                if (p_head == mp_head)
                {
                    nrf_802154_lp_timer_start(t0, dt);
                }
            }

            mutex_unlock(&m_timer_mutex);
        }
    }
    while (queue_cntr != m_queue_changed_cntr);
}

/**
 * @brief Remove timer from the queue
 *
 * @param [inout]  p_timer  Pointer to timer to remove from the queue.
 *
 * @retval true   @sa handle_timer() shall be called by caller of this function.
 * @retval false  @sa handle_timer() shall not be called by the caller.
 */
static bool timer_remove(nrf_802154_timer_t * p_timer)
{
    assert(p_timer != NULL);

    nrf_802154_timer_t         ** pp_item;
    nrf_802154_timer_t * volatile p_next; // Volatile pointer to prevent compiler from removing any code related to this variable during optimization (IAR).
    nrf_802154_timer_t          * p_cur;
    uint8_t                       queue_cntr;
    bool                          timer_start;
    bool                          timer_stop;

    while (true)
    {
        queue_cntr  = m_queue_changed_cntr;
        pp_item     = (nrf_802154_timer_t **)&mp_head;
        p_next      = NULL;
        p_cur       = NULL;
        timer_start = false;
        timer_stop  = false;

        // Find entry to remove
        while (true)
        {
            p_cur = (nrf_802154_timer_t *)__LDREXW((uint32_t *)pp_item);

            if ((p_cur == NULL) || (p_cur == p_timer))
            {
                break;
            }

            pp_item = &(p_cur->p_next);
        }

        if (queue_cntr != m_queue_changed_cntr)
        {
            // Higher priority modified the queue while iterating, try again.
            continue;
        }

        if (p_cur == p_timer)
        {
            // Entry found.
            p_next = p_cur->p_next;

            // Restart timer when removing HEAD and other timer instance is pending.
            if (p_cur == mp_head)
            {
                if (p_next != NULL)
                {
                    timer_start = true;
                }
                else
                {
                    timer_stop = true;
                }
            }
        }
        else
        {
            // Entry not found
            __CLREX();
            break;
        }

        if (!__STREXW((uint32_t)p_next, (uint32_t *)pp_item))
        {
            // Exit, if exclusive access succeeds.
            queue_cntr_bump();
            break;
        }
    }

    // Write to the pointer next on removal to ensure that node removal is detected by
    // lower pritority context in case it was going to be used.
    if (p_cur != NULL)
    {
        uint32_t temp;

        do
        {
            // This assignment is used to prevent compiler from removing exclusive load during optimization (IAR).
            temp = __LDREXW((uint32_t *)&p_cur->p_next);
            assert((void *)temp != p_cur);
        }
        while (__STREXW(temp, (uint32_t *)&p_cur->p_next));
    }

    return (timer_start || timer_stop);
}

void nrf_802154_timer_sched_init(void)
{
    mp_head              = NULL;
    m_timer_mutex        = 0;
    m_fired_mutex        = 0;
    m_queue_changed_cntr = 0;
}

void nrf_802154_timer_sched_deinit(void)
{
    nrf_802154_lp_timer_stop();

    mp_head = NULL;
}

uint32_t nrf_802154_timer_sched_time_get(void)
{
    return nrf_802154_lp_timer_time_get();
}

uint32_t nrf_802154_timer_sched_granularity_get(void)
{
    return nrf_802154_lp_timer_granularity_get();
}

bool nrf_802154_timer_sched_time_is_in_future(uint32_t now, uint32_t t0, uint32_t dt)
{
    uint32_t target_time = t0 + dt;
    int32_t  difference  = target_time - now;

    return difference > 0;
}

uint32_t nrf_802154_timer_sched_remaining_time_get(const nrf_802154_timer_t * p_timer)
{
    assert(p_timer != NULL);

    uint32_t now        = nrf_802154_lp_timer_time_get();
    uint32_t expiration = p_timer->t0 + p_timer->dt;
    int32_t  remaining  = expiration - now;

    if (remaining > 0)
    {
        return (uint32_t)remaining;
    }
    else
    {
        return 0ul;
    }
}

void nrf_802154_timer_sched_add(nrf_802154_timer_t * p_timer, bool round_up)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_TSCH_ADD);

    assert(p_timer != NULL);
    assert(p_timer->callback != NULL);

    if (round_up)
    {
        p_timer->dt += nrf_802154_lp_timer_granularity_get() - 1;
    }

    if (timer_remove(p_timer))
    {
        handle_timer();
    }

    nrf_802154_timer_t ** pp_item;
    nrf_802154_timer_t  * p_next;
    uint8_t               queue_cntr;

    while (true)
    {
        queue_cntr = m_queue_changed_cntr;
        pp_item    = (nrf_802154_timer_t **)&mp_head;
        p_next     = NULL;

        // Search the current queue to find appropriate position to insert timer.
        while (true)
        {
            nrf_802154_timer_t * p_cur = (nrf_802154_timer_t *)__LDREXW((uint32_t *)pp_item);

            assert(p_cur != p_timer);

            if (p_cur == NULL)
            {
                // No HEAD or insert at the end.
                p_next = NULL;
                break;
            }

            if (is_timer_prior(p_timer, p_cur))
            {
                // Insert at the beginning with existing HEAD or somewhere in the middle.
                p_next = p_cur;
                break;
            }

            pp_item = &(p_cur->p_next);
        }

        if (queue_cntr != m_queue_changed_cntr)
        {
            // Higher priority modified the queue while iterating, try again.
            continue;
        }

        assert(p_next != p_timer);
        p_timer->p_next = p_next;

        if (!__STREXW((uint32_t)p_timer, (uint32_t *)pp_item))
        {
            // Exit, if exclusive access succeeds.
            queue_cntr_bump();
            break;
        }
    }

    if (mp_head == p_timer)
    {
        handle_timer();
    }

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_TSCH_ADD);
}

void nrf_802154_timer_sched_remove(nrf_802154_timer_t * p_timer)
{
    if (timer_remove(p_timer))
    {
        handle_timer();
    }
}

bool nrf_802154_timer_sched_is_running(nrf_802154_timer_t * p_timer)
{
    uint8_t queue_cntr;
    bool    result;

    do
    {
        result     = false;
        queue_cntr = m_queue_changed_cntr;

        for (volatile nrf_802154_timer_t * p_cur = mp_head;
             p_cur != NULL;
             p_cur = p_cur->p_next)
        {
            if (p_cur == p_timer)
            {
                result = true;
                break;
            }
        }
    }
    while (queue_cntr != m_queue_changed_cntr);

    return result;
}

void nrf_802154_lp_timer_fired(void)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_TSCH_FIRED);

    if (mutex_trylock(&m_fired_mutex))
    {
        nrf_802154_timer_t * p_timer = (nrf_802154_timer_t *)mp_head;

        if (p_timer != NULL)
        {
            nrf_802154_timer_callback_t callback  = p_timer->callback;
            void                      * p_context = p_timer->p_context;

            (void)timer_remove(p_timer);

            if (callback != NULL)
            {
                callback(p_context);
            }
        }

        mutex_unlock(&m_fired_mutex);
    }

    handle_timer();

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_TSCH_FIRED);
}
