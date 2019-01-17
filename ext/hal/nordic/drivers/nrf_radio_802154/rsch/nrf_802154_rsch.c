#include "nrf_802154_rsch.h"

#include <assert.h>
#include <stddef.h>
#include <nrf.h>

#include "nrf_802154_debug.h"
#include "nrf_802154_priority_drop.h"
#include "platform/clock/nrf_802154_clock.h"
#include "raal/nrf_raal_api.h"
#include "timer_scheduler/nrf_802154_timer_sched.h"

#define PREC_RAMP_UP_TIME 300                                ///< Ramp-up time of preconditions [us]. 300 is worst case for HFclock

static volatile uint8_t     m_ntf_mutex;                     ///< Mutex for notyfying core.
static volatile uint8_t     m_ntf_mutex_monitor;             ///< Mutex monitor, incremented every failed ntf mutex lock.
static volatile uint8_t     m_req_mutex;                     ///< Mutex for requesting preconditions.
static volatile uint8_t     m_req_mutex_monitor;             ///< Mutex monitor, incremented every failed req mutex lock.
static volatile rsch_prio_t m_last_notified_prio;            ///< Last reported approved priority level.
static volatile rsch_prio_t m_approved_prios[RSCH_PREC_CNT]; ///< Priority levels approved by each precondition.
static rsch_prio_t          m_requested_prio;                ///< Priority requested from all preconditions.
static rsch_prio_t          m_cont_mode_prio;                ///< Continuous mode priority level. If continuous mode is not requested equal to @ref RSCH_PRIO_IDLE.

typedef struct
{
    rsch_prio_t        prio;  ///< Delayed timeslot priority level. If delayed timeslot is not scheduled equal to @ref RSCH_PRIO_IDLE.
    uint32_t           t0;    ///< Time base of the delayed timeslot trigger time.
    uint32_t           dt;    ///< Time delta of the delayed timeslot trigger time.
    nrf_802154_timer_t timer; ///< Timer used to trigger delayed timeslot.
} dly_ts_t;

static dly_ts_t m_dly_ts[RSCH_DLY_TS_NUM];

/** @brief Non-blocking mutex for notifying core.
 *
 *  @param[inout]  p_mutex          Pointer to the mutex data.
 *  @param[inout]  p_mutex_monitor  Pointer to the mutex monitor counter.
 *
 *  @retval  true   Mutex was acquired.
 *  @retval  false  Mutex could not be acquired.
 */
static inline bool mutex_trylock(volatile uint8_t * p_mutex, volatile uint8_t * p_mutex_monitor)
{
    do
    {
        uint8_t mutex_value = __LDREXB(p_mutex);

        if (mutex_value)
        {
            __CLREX();

            (*p_mutex_monitor)++;
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

/** @brief Check maximal priority level required by any of delayed timeslots at the moment.
 *
 * To meet delayed timeslot timing requirements there is a time window in which radio
 * preconditions should be requested. This function is used to prevent releasing preconditions
 * in this time window.
 *
 * @return  Maximal priority level required by delayed timeslots.
 */
static rsch_prio_t max_prio_for_delayed_timeslot_get(void)
{
    rsch_prio_t result = RSCH_PRIO_IDLE;
    uint32_t    now    = nrf_802154_timer_sched_time_get();

    for (uint32_t i = 0; i < RSCH_DLY_TS_NUM; i++)
    {
        dly_ts_t * p_dly_ts = &m_dly_ts[i];
        uint32_t   t0       = p_dly_ts->t0;
        uint32_t   dt       = p_dly_ts->dt - PREC_RAMP_UP_TIME -
                              nrf_802154_timer_sched_granularity_get();

        if ((p_dly_ts->prio > result) && !nrf_802154_timer_sched_time_is_in_future(now, t0, dt))
        {
            result = p_dly_ts->prio;
        }
    }

    return result;
}

static rsch_prio_t required_prio_lvl_get(void)
{
    rsch_prio_t result = max_prio_for_delayed_timeslot_get();

    if (m_cont_mode_prio > result)
    {
        result = m_cont_mode_prio;
    }

    return result;
}

/** @brief Set approved priority level @p prio on given precondition @p prec.
 *
 * When requested priority level equals to the @ref RSCH_PRIO_IDLE this function will approve only
 * the @ref RSCH_PRIO_IDLE priority level and drop other approved levels silently.
 *
 * @param[in]  prec    Precondition which state will be changed.
 * @param[in]  prio    Approved priority level for given precondition.
 */
static inline void prec_approved_prio_set(rsch_prec_t prec, rsch_prio_t prio)
{
    assert(prec <= RSCH_PREC_CNT);

    if ((m_requested_prio == RSCH_PRIO_IDLE) && (prio != RSCH_PRIO_IDLE))
    {
        // Ignore approved precondition - it was not requested.
        return;
    }

    assert((m_approved_prios[prec] != prio) || (prio == RSCH_PRIO_IDLE));

    m_approved_prios[prec] = prio;
}

/** @brief Request all preconditions.
 */
static inline void all_prec_update(void)
{
    rsch_prio_t prev_prio;
    rsch_prio_t new_prio;
    uint8_t     monitor;

    do
    {
        if (!mutex_trylock(&m_req_mutex, &m_req_mutex_monitor))
        {
            return;
        }

        monitor   = m_req_mutex_monitor;
        prev_prio = m_requested_prio;
        new_prio  = required_prio_lvl_get();

        if (prev_prio != new_prio)
        {
            m_requested_prio = new_prio;

            if (new_prio == RSCH_PRIO_IDLE)
            {
                nrf_802154_priority_drop_hfclk_stop();
                prec_approved_prio_set(RSCH_PREC_HFCLK, RSCH_PRIO_IDLE);

                nrf_raal_continuous_mode_exit();
                prec_approved_prio_set(RSCH_PREC_RAAL, RSCH_PRIO_IDLE);
            }
            else
            {
                nrf_802154_priority_drop_hfclk_stop_terminate();
                nrf_802154_clock_hfclk_start();
                nrf_raal_continuous_mode_enter();
            }
        }

        mutex_unlock(&m_req_mutex);
    }
    while (monitor != m_req_mutex_monitor);
}

/** @brief Get currently approved priority level.
 *
 * @return Maximal priority level approved by all radio preconditions.
 */
static inline rsch_prio_t approved_prio_lvl_get(void)
{
    rsch_prio_t result = RSCH_PRIO_MAX;

    for (uint32_t i = 0; i < RSCH_PREC_CNT; i++)
    {
        if (m_approved_prios[i] < result)
        {
            result = m_approved_prios[i];
        }
    }

    return result;
}

/** @brief Check if all preconditions are requested or met at given priority level or higher.
 *
 * @param[in]  prio  Minimal priority level requested from preconditions.
 *
 * @retval true   All preconditions are requested or met at given or higher level.
 * @retval false  At least one precondition is requested at lower level than required.
 */
static inline bool requested_prio_lvl_is_at_least(rsch_prio_t prio)
{
    return m_requested_prio >= prio;
}

/** @brief Notify core if preconditions are approved or denied if current state differs from last reported.
 */
static inline void notify_core(void)
{
    rsch_prio_t approved_prio_lvl;
    uint8_t     temp_mon;

    do
    {
        if (!mutex_trylock(&m_ntf_mutex, &m_ntf_mutex_monitor))
        {
            return;
        }

        /* It is possible that preemption is not detected (m_ntf_mutex_monitor is read after
         * acquiring mutex). It is not a problem because we will call proper handler function
         * requested by preempting context. Avoiding this race would generate one additional
         * iteration without any effect.
         */
        temp_mon          = m_ntf_mutex_monitor;
        approved_prio_lvl = approved_prio_lvl_get();

        if ((m_cont_mode_prio > RSCH_PRIO_IDLE) && (m_last_notified_prio != approved_prio_lvl))
        {
            m_last_notified_prio = approved_prio_lvl;

            nrf_802154_rsch_continuous_prio_changed(approved_prio_lvl);
        }

        mutex_unlock(&m_ntf_mutex);
    }
    while (temp_mon != m_ntf_mutex_monitor);
}

/** Timer callback used to trigger delayed timeslot.
 *
 * @param[in]  p_context  Index of the delayed timeslot operation (TX or RX).
 */
static void delayed_timeslot_start(void * p_context)
{
    rsch_dly_ts_id_t dly_ts_id = (rsch_dly_ts_id_t)(uint32_t)p_context;
    dly_ts_t       * p_dly_ts  = &m_dly_ts[dly_ts_id];
    rsch_prio_t      req_prio_lvl;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_TIMER_DELAYED_START);

    req_prio_lvl   = p_dly_ts->prio;
    p_dly_ts->prio = RSCH_PRIO_IDLE;

    if (approved_prio_lvl_get() >= req_prio_lvl)
    {
        nrf_802154_rsch_delayed_timeslot_started(dly_ts_id);
    }
    else
    {
        nrf_802154_rsch_delayed_timeslot_failed(dly_ts_id);
    }

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_TIMER_DELAYED_START);
}

/** Timer callback used to request preconditions for delayed timeslot.
 *
 * @param[in]  p_context  Index of the delayed timeslot operation (TX or RX).
 */
static void delayed_timeslot_prec_request(void * p_context)
{
    rsch_dly_ts_id_t dly_ts_id = (rsch_dly_ts_id_t)(uint32_t)p_context;
    dly_ts_t       * p_dly_ts  = &m_dly_ts[dly_ts_id];

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_TIMER_DELAYED_PREC);

    all_prec_update();

    p_dly_ts->timer.t0        = p_dly_ts->t0;
    p_dly_ts->timer.dt        = p_dly_ts->dt;
    p_dly_ts->timer.callback  = delayed_timeslot_start;
    p_dly_ts->timer.p_context = p_context;

    nrf_802154_timer_sched_add(&p_dly_ts->timer, true);

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_TIMER_DELAYED_PREC);
}

/***************************************************************************************************
 * Public API
 **************************************************************************************************/

void nrf_802154_rsch_init(void)
{
    nrf_raal_init();

    m_ntf_mutex          = 0;
    m_req_mutex          = 0;
    m_last_notified_prio = RSCH_PRIO_IDLE;
    m_cont_mode_prio     = RSCH_PRIO_IDLE;
    m_requested_prio     = RSCH_PRIO_IDLE;

    for (uint32_t i = 0; i < RSCH_DLY_TS_NUM; i++)
    {
        m_dly_ts[i].prio = RSCH_PRIO_IDLE;
    }

    for (uint32_t i = 0; i < RSCH_PREC_CNT; i++)
    {
        m_approved_prios[i] = RSCH_PRIO_IDLE;
    }
}

void nrf_802154_rsch_uninit(void)
{
    for (uint32_t i = 0; i < RSCH_DLY_TS_NUM; i++)
    {
        nrf_802154_timer_sched_remove(&m_dly_ts[i].timer);
    }

    nrf_raal_uninit();
}

void nrf_802154_rsch_continuous_mode_priority_set(rsch_prio_t prio)
{
    nrf_802154_log(EVENT_TRACE_ENTER, (prio > RSCH_PRIO_IDLE) ? FUNCTION_RSCH_CONTINUOUS_ENTER :
                   FUNCTION_RSCH_CONTINUOUS_EXIT);

    m_cont_mode_prio = prio;
    __DMB();

    all_prec_update();
    notify_core();

    if (prio == RSCH_PRIO_IDLE)
    {
        m_last_notified_prio = RSCH_PRIO_IDLE;
    }

    nrf_802154_log(EVENT_TRACE_EXIT, (prio > RSCH_PRIO_IDLE) ? FUNCTION_RSCH_CONTINUOUS_ENTER :
                   FUNCTION_RSCH_CONTINUOUS_EXIT);
}

void nrf_802154_rsch_continuous_ended(void)
{
    nrf_raal_continuous_ended();
}

bool nrf_802154_rsch_timeslot_request(uint32_t length_us)
{
    return nrf_raal_timeslot_request(length_us);
}

bool nrf_802154_rsch_delayed_timeslot_request(uint32_t         t0,
                                              uint32_t         dt,
                                              uint32_t         length,
                                              rsch_prio_t      prio,
                                              rsch_dly_ts_id_t dly_ts_id)
{
    (void)length;

    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_DELAYED_TIMESLOT_REQ);
    assert(dly_ts_id < RSCH_DLY_TS_NUM);

    dly_ts_t * p_dly_ts = &m_dly_ts[dly_ts_id];
    uint32_t   now      = nrf_802154_timer_sched_time_get();
    uint32_t   req_dt   = dt - PREC_RAMP_UP_TIME;
    bool       result;

    assert(!nrf_802154_timer_sched_is_running(&p_dly_ts->timer));
    assert(p_dly_ts->prio == RSCH_PRIO_IDLE);
    assert(prio != RSCH_PRIO_IDLE);

    if (nrf_802154_timer_sched_time_is_in_future(now, t0, req_dt))
    {
        p_dly_ts->prio = prio;
        p_dly_ts->t0   = t0;
        p_dly_ts->dt   = dt;

        p_dly_ts->timer.t0        = t0;
        p_dly_ts->timer.dt        = req_dt;
        p_dly_ts->timer.callback  = delayed_timeslot_prec_request;
        p_dly_ts->timer.p_context = (void *)dly_ts_id;

        nrf_802154_timer_sched_add(&p_dly_ts->timer, false);

        result = true;
    }
    else if (requested_prio_lvl_is_at_least(RSCH_PRIO_MAX) &&
             nrf_802154_timer_sched_time_is_in_future(now, t0, dt))
    {
        p_dly_ts->prio = prio;
        p_dly_ts->t0   = t0;
        p_dly_ts->dt   = dt;

        p_dly_ts->timer.t0        = t0;
        p_dly_ts->timer.dt        = dt;
        p_dly_ts->timer.callback  = delayed_timeslot_start;
        p_dly_ts->timer.p_context = (void *)dly_ts_id;

        nrf_802154_timer_sched_add(&p_dly_ts->timer, true);

        result = true;
    }
    else
    {
        result = false;
    }

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_DELAYED_TIMESLOT_REQ);

    return result;
}

bool nrf_802154_rsch_prec_is_approved(rsch_prec_t prec, rsch_prio_t prio)
{
    assert(prec < RSCH_PREC_CNT);
    return m_approved_prios[prec] >= prio;
}

uint32_t nrf_802154_rsch_timeslot_us_left_get(void)
{
    return nrf_raal_timeslot_us_left_get();
}

// External handlers

void nrf_raal_timeslot_started(void)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_TIMESLOT_STARTED);

    prec_approved_prio_set(RSCH_PREC_RAAL, RSCH_PRIO_MAX);
    notify_core();

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_TIMESLOT_STARTED);
}

void nrf_raal_timeslot_ended(void)
{
    nrf_802154_log(EVENT_TRACE_ENTER, FUNCTION_RSCH_TIMESLOT_ENDED);

    prec_approved_prio_set(RSCH_PREC_RAAL, RSCH_PRIO_IDLE);
    notify_core();

    nrf_802154_log(EVENT_TRACE_EXIT, FUNCTION_RSCH_TIMESLOT_ENDED);
}

void nrf_802154_clock_hfclk_ready(void)
{
    prec_approved_prio_set(RSCH_PREC_HFCLK, RSCH_PRIO_MAX);
    notify_core();
}
