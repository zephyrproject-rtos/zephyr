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
 *   This file implements SWI manager for nRF 802.15.4 driver.
 *
 */

#include "nrf_802154_swi.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "nrf_802154.h"
#include "nrf_802154_config.h"
#include "nrf_802154_core.h"
#include "nrf_802154_rx_buffer.h"
#include "hal/nrf_egu.h"
#include "platform/clock/nrf_802154_clock.h"

/** Size of notification queue.
 *
 * One slot for each receive buffer, one for transmission, one for busy channel and one for energy
 * detection.
 */
#define NTF_QUEUE_SIZE     (NRF_802154_RX_BUFFERS + 3)
/** Size of requests queue.
 *
 * Two is minimal queue size. It is not expected in current implementation to queue a few requests.
 */
#define REQ_QUEUE_SIZE     2

#define SWI_EGU            NRF_802154_SWI_EGU_INSTANCE ///< Label of SWI peripheral.
#define SWI_IRQn           NRF_802154_SWI_IRQN         ///< Symbol of SWI IRQ number.
#define SWI_IRQHandler     NRF_802154_SWI_IRQ_HANDLER  ///< Symbol of SWI IRQ handler.

#define NTF_INT            NRF_EGU_INT_TRIGGERED0      ///< Label of notification interrupt.
#define NTF_TASK           NRF_EGU_TASK_TRIGGER0       ///< Label of notification task.
#define NTF_EVENT          NRF_EGU_EVENT_TRIGGERED0    ///< Label of notification event.

#define HFCLK_STOP_INT     NRF_EGU_INT_TRIGGERED1      ///< Label of HFClk stop interrupt.
#define HFCLK_STOP_TASK    NRF_EGU_TASK_TRIGGER1       ///< Label of HFClk stop task.
#define HFCLK_STOP_EVENT   NRF_EGU_EVENT_TRIGGERED1    ///< Label of HFClk stop event.

#define REQ_INT            NRF_EGU_INT_TRIGGERED2      ///< Label of request interrupt.
#define REQ_TASK           NRF_EGU_TASK_TRIGGER2       ///< Label of request task.
#define REQ_EVENT          NRF_EGU_EVENT_TRIGGERED2    ///< Label of request event.

#define RAW_LENGTH_OFFSET  0
#define RAW_PAYLOAD_OFFSET 1

/// Types of notifications in notification queue.
typedef enum
{
    NTF_TYPE_RECEIVED,                ///< Frame received
    NTF_TYPE_RECEIVE_FAILED,          ///< Frame reception failed
    NTF_TYPE_TRANSMITTED,             ///< Frame transmitted
    NTF_TYPE_TRANSMIT_FAILED,         ///< Frame transmission failure
    NTF_TYPE_ENERGY_DETECTED,         ///< Energy detection procedure ended
    NTF_TYPE_ENERGY_DETECTION_FAILED, ///< Energy detection procedure failed
    NTF_TYPE_CCA,                     ///< CCA procedure ended
    NTF_TYPE_CCA_FAILED,              ///< CCA procedure failed
} nrf_802154_ntf_type_t;

/// Notification data in the notification queue.
typedef struct
{
    nrf_802154_ntf_type_t type; ///< Notification type.

    union
    {
        struct
        {
            uint8_t * p_psdu; ///< Pointer to received frame PSDU.
            int8_t    power;  ///< RSSI of received frame.
            int8_t    lqi;    ///< LQI of received frame.
        } received;           ///< Received frame details.

        struct
        {
            nrf_802154_rx_error_t error; ///< An error code that indicates reason of the failure.
        } receive_failed;

        struct
        {
            const uint8_t * p_frame; ///< Pointer to frame that was transmitted.
            uint8_t       * p_psdu;  ///< Pointer to received ACK PSDU or NULL.
            int8_t          power;   ///< RSSI of received ACK or 0.
            int8_t          lqi;     ///< LQI of received ACK or 0.
        } transmitted;               ///< Transmitted frame details.

        struct
        {
            const uint8_t       * p_frame; ///< Pointer to frame that was requested to be transmitted, but failed.
            nrf_802154_tx_error_t error;   ///< An error code that indicates reason of the failure.
        } transmit_failed;

        struct
        {
            int8_t result; ///< Energy detection result.
        } energy_detected; ///< Energy detection details.

        struct
        {
            nrf_802154_ed_error_t error; ///< An error code that indicates reason of the failure.
        } energy_detection_failed;       ///< Energy detection failure details.

        struct
        {
            bool result; ///< CCA result.
        } cca;           ///< CCA details.

        struct
        {
            nrf_802154_cca_error_t error; ///< An error code that indicates reason of the failure.
        } cca_failed;                     ///< CCA failure details.
    } data;                               ///< Notification data depending on it's type.
} nrf_802154_ntf_data_t;

/// Type of requests in request queue.
typedef enum
{
    REQ_TYPE_SLEEP,
    REQ_TYPE_RECEIVE,
    REQ_TYPE_TRANSMIT,
    REQ_TYPE_ENERGY_DETECTION,
    REQ_TYPE_CCA,
    REQ_TYPE_CONTINUOUS_CARRIER,
    REQ_TYPE_BUFFER_FREE,
    REQ_TYPE_CHANNEL_UPDATE,
    REQ_TYPE_CCA_CFG_UPDATE
} nrf_802154_req_type_t;

/// Request data in request queue.
typedef struct
{
    nrf_802154_req_type_t type; ///< Type of the request.

    union
    {
        struct
        {
            nrf_802154_term_t term_lvl; ///< Request priority.
            bool            * p_result; ///< Sleep request result.
        } sleep;                        ///< Sleep request details.

        struct
        {
            nrf_802154_notification_func_t notif_func;  ///< Error notified in case of success.
            nrf_802154_term_t              term_lvl;    ///< Request priority.
            req_originator_t               req_orig;    ///< Request originator.
            bool                           notif_abort; ///< If function termination should be notified.
            bool                         * p_result;    ///< Receive request result.
        } receive;                                      ///< Receive request details.

        struct
        {
            nrf_802154_notification_func_t notif_func; ///< Error notified in case of success.
            nrf_802154_term_t              term_lvl;   ///< Request priority.
            req_originator_t               req_orig;   ///< Request originator.
            const uint8_t                * p_data;     ///< Pointer to PSDU to transmit.
            bool                           cca;        ///< If CCA was requested prior to transmission.
            bool                           immediate;  ///< If TX procedure must be performed immediately.
            bool                         * p_result;   ///< Transmit request result.
        } transmit;                                    ///< Transmit request details.

        struct
        {
            nrf_802154_term_t term_lvl; ///< Request priority.
            bool            * p_result; ///< Energy detection request result.
            uint32_t          time_us;  ///< Requested time of energy detection procedure.
        } energy_detection;             ///< Energy detection request details.

        struct
        {
            nrf_802154_term_t term_lvl; ///< Request priority.
            bool            * p_result; ///< CCA request result.
        } cca;                          ///< CCA request details.

        struct
        {
            nrf_802154_term_t term_lvl; ///< Request priority.
            bool            * p_result; ///< Continuous carrier request result.
        } continuous_carrier;           ///< Continuous carrier request details.

        struct
        {
            uint8_t * p_data;   ///< Pointer to receive buffer to free.
            bool    * p_result; ///< Buffer free request result.
        } buffer_free;          ///< Buffer free request details.

        struct
        {
            bool * p_result; ///< Channel update request result.
        } channel_update;    ///< Channel update request details.

        struct
        {
            bool * p_result;                              ///< CCA config update request result.
        } cca_cfg_update;                                 ///< CCA config update request details.
    } data;                                               ///< Request data depending on it's type.
} nrf_802154_req_data_t;

static nrf_802154_ntf_data_t m_ntf_queue[NTF_QUEUE_SIZE]; ///< Notification queue.
static uint8_t               m_ntf_r_ptr;                 ///< Notification queue read index.
static uint8_t               m_ntf_w_ptr;                 ///< Notification queue write index.

static nrf_802154_req_data_t m_req_queue[REQ_QUEUE_SIZE]; ///< Request queue.
static uint8_t               m_req_r_ptr;                 ///< Request queue read index.
static uint8_t               m_req_w_ptr;                 ///< Request queue write index.

/**
 * Increment given index for any queue.
 *
 * @param[inout]  p_ptr       Index to increment.
 * @param[in]     queue_size  Number of elements in the queue.
 */
static void queue_ptr_increment(uint8_t * p_ptr, uint8_t queue_size)
{
    if (++(*p_ptr) >= queue_size)
    {
        *p_ptr = 0;
    }
}

/**
 * Check if given queue is full.
 *
 * @param[in]  r_ptr       Read index associated with given queue.
 * @param[in]  w_ptr       Write index associated with given queue.
 * @param[in]  queue_size  Number of elements in the queue.
 *
 * @retval  true   Given queue is full.
 * @retval  false  Given queue is not full.
 */
static bool queue_is_full(uint8_t r_ptr, uint8_t w_ptr, uint8_t queue_size)
{
    if (w_ptr == (r_ptr - 1))
    {
        return true;
    }

    if ((r_ptr == 0) && (w_ptr == queue_size - 1))
    {
        return true;
    }

    return false;
}

/**
 * Check if given queue is empty.
 *
 * @param[in]  r_ptr  Read index associated with given queue.
 * @param[in]  w_ptr  Write index associated with given queue.
 *
 * @retval  true   Given queue is empty.
 * @retval  false  Given queue is not empty.
 */
static bool queue_is_empty(uint8_t r_ptr, uint8_t w_ptr)
{
    return (r_ptr == w_ptr);
}

/**
 * Increment given index associated with notification queue.
 *
 * @param[inout]  p_ptr  Pointer to the index to increment.
 */
static void ntf_queue_ptr_increment(uint8_t * p_ptr)
{
    queue_ptr_increment(p_ptr, NTF_QUEUE_SIZE);
}

/**
 * Check if notification queue is full.
 *
 * @retval  true   Notification queue is full.
 * @retval  false  Notification queue is not full.
 */
static bool ntf_queue_is_full(void)
{
    return queue_is_full(m_ntf_r_ptr, m_ntf_w_ptr, NTF_QUEUE_SIZE);
}

/**
 * Check if notification queue is empty.
 *
 * @retval  true   Notification queue is empty.
 * @retval  false  Notification queue is not empty.
 */
static bool ntf_queue_is_empty(void)
{
    return queue_is_empty(m_ntf_r_ptr, m_ntf_w_ptr);
}

/**
 * Enter notify block.
 *
 * This is a helper function used in all notification functions to atomically
 * find an empty slot in the notification queue and allow atomic slot update.
 *
 * @return Pointer to an empty slot in the notification queue.
 */
static nrf_802154_ntf_data_t * ntf_enter(void)
{
    __disable_irq();
    __DSB();
    __ISB();

    assert(!ntf_queue_is_full());
    (void)ntf_queue_is_full();

    return &m_ntf_queue[m_ntf_w_ptr];
}

/**
 * Exit notify block.
 *
 * This is a helper function used in all notification functions to end atomic slot update
 * and trigger SWI to process the notification from the slot.
 */
static void ntf_exit(void)
{
    ntf_queue_ptr_increment(&m_ntf_w_ptr);

    nrf_egu_task_trigger(SWI_EGU, NTF_TASK);

    __enable_irq();
}

/**
 * Increment given index associated with request queue.
 *
 * @param[inout]  p_ptr  Pointer to the index to increment.
 */
static void req_queue_ptr_increment(uint8_t * p_ptr)
{
    queue_ptr_increment(p_ptr, REQ_QUEUE_SIZE);
}

/**
 * Check if request queue is full.
 *
 * @retval  true   Request queue is full.
 * @retval  false  Request queue is not full.
 */
static bool req_queue_is_full(void)
{
    return queue_is_full(m_req_r_ptr, m_req_w_ptr, REQ_QUEUE_SIZE);
}

/**
 * Check if request queue is empty.
 *
 * @retval  true   Request queue is empty.
 * @retval  false  Request queue is not empty.
 */
static bool req_queue_is_empty(void)
{
    return queue_is_empty(m_req_r_ptr, m_req_w_ptr);
}

/**
 * Enter request block.
 *
 * This is a helper function used in all request functions to atomically
 * find an empty slot in request queue and allow atomic slot update.
 *
 * @return Pointer to an empty slot in the request queue.
 */
static nrf_802154_req_data_t * req_enter(void)
{
    __disable_irq();
    __DSB();
    __ISB();

    assert(!req_queue_is_full());
    (void)req_queue_is_full();

    return &m_req_queue[m_req_w_ptr];
}

/**
 * Exit request block.
 *
 * This is a helper function used in all request functions to end atomic slot update
 * and trigger SWI to process the request from the slot.
 */
static void req_exit(void)
{
    req_queue_ptr_increment(&m_req_w_ptr);

    nrf_egu_task_trigger(SWI_EGU, REQ_TASK);

    __enable_irq();
    __DSB();
    __ISB();
}

void nrf_802154_swi_init(void)
{
    m_ntf_r_ptr = 0;
    m_ntf_w_ptr = 0;

    nrf_egu_int_enable(SWI_EGU, NTF_INT | HFCLK_STOP_INT | REQ_INT);

    NVIC_SetPriority(SWI_IRQn, NRF_802154_SWI_PRIORITY);
    NVIC_ClearPendingIRQ(SWI_IRQn);
    NVIC_EnableIRQ(SWI_IRQn);
}

void nrf_802154_swi_notify_received(uint8_t * p_data, int8_t power, int8_t lqi)
{
    nrf_802154_ntf_data_t * p_slot = ntf_enter();

    p_slot->type                 = NTF_TYPE_RECEIVED;
    p_slot->data.received.p_psdu = p_data;
    p_slot->data.received.power  = power;
    p_slot->data.received.lqi    = lqi;

    ntf_exit();
}

void nrf_802154_swi_notify_receive_failed(nrf_802154_rx_error_t error)
{
    nrf_802154_ntf_data_t * p_slot = ntf_enter();

    p_slot->type                      = NTF_TYPE_RECEIVE_FAILED;
    p_slot->data.receive_failed.error = error;

    ntf_exit();
}

void nrf_802154_swi_notify_transmitted(const uint8_t * p_frame,
                                       uint8_t       * p_data,
                                       int8_t          power,
                                       int8_t          lqi)
{
    nrf_802154_ntf_data_t * p_slot = ntf_enter();

    p_slot->type                     = NTF_TYPE_TRANSMITTED;
    p_slot->data.transmitted.p_frame = p_frame;
    p_slot->data.transmitted.p_psdu  = p_data;
    p_slot->data.transmitted.power   = power;
    p_slot->data.transmitted.lqi     = lqi;

    ntf_exit();
}

void nrf_802154_swi_notify_transmit_failed(const uint8_t * p_frame, nrf_802154_tx_error_t error)
{
    nrf_802154_ntf_data_t * p_slot = ntf_enter();

    p_slot->type                         = NTF_TYPE_TRANSMIT_FAILED;
    p_slot->data.transmit_failed.p_frame = p_frame;
    p_slot->data.transmit_failed.error   = error;

    ntf_exit();
}

void nrf_802154_swi_notify_energy_detected(uint8_t result)
{
    nrf_802154_ntf_data_t * p_slot = ntf_enter();

    p_slot->type                        = NTF_TYPE_ENERGY_DETECTED;
    p_slot->data.energy_detected.result = result;

    ntf_exit();
}

void nrf_802154_swi_notify_energy_detection_failed(nrf_802154_ed_error_t error)
{
    nrf_802154_ntf_data_t * p_slot = ntf_enter();

    p_slot->type                               = NTF_TYPE_ENERGY_DETECTION_FAILED;
    p_slot->data.energy_detection_failed.error = error;

    ntf_exit();
}

void nrf_802154_swi_notify_cca(bool channel_free)
{
    nrf_802154_ntf_data_t * p_slot = ntf_enter();

    p_slot->type            = NTF_TYPE_CCA;
    p_slot->data.cca.result = channel_free;

    ntf_exit();
}

void nrf_802154_swi_notify_cca_failed(nrf_802154_cca_error_t error)
{
    nrf_802154_ntf_data_t * p_slot = ntf_enter();

    p_slot->type                  = NTF_TYPE_CCA_FAILED;
    p_slot->data.cca_failed.error = error;

    ntf_exit();
}

void nrf_802154_swi_hfclk_stop(void)
{
    assert(!nrf_egu_event_check(SWI_EGU, HFCLK_STOP_EVENT));

    nrf_egu_task_trigger(SWI_EGU, HFCLK_STOP_TASK);
}

void nrf_802154_swi_hfclk_stop_terminate(void)
{
    nrf_egu_event_clear(SWI_EGU, HFCLK_STOP_EVENT);
}

void nrf_802154_swi_sleep(nrf_802154_term_t term_lvl, bool * p_result)
{
    nrf_802154_req_data_t * p_slot = req_enter();

    p_slot->type                = REQ_TYPE_SLEEP;
    p_slot->data.sleep.term_lvl = term_lvl;
    p_slot->data.sleep.p_result = p_result;

    req_exit();
}

void nrf_802154_swi_receive(nrf_802154_term_t              term_lvl,
                            req_originator_t               req_orig,
                            nrf_802154_notification_func_t notify_function,
                            bool                           notify_abort,
                            bool                         * p_result)
{
    nrf_802154_req_data_t * p_slot = req_enter();

    p_slot->type                     = REQ_TYPE_RECEIVE;
    p_slot->data.receive.term_lvl    = term_lvl;
    p_slot->data.receive.req_orig    = req_orig;
    p_slot->data.receive.notif_func  = notify_function;
    p_slot->data.receive.notif_abort = notify_abort;
    p_slot->data.receive.p_result    = p_result;

    req_exit();
}

void nrf_802154_swi_transmit(nrf_802154_term_t              term_lvl,
                             req_originator_t               req_orig,
                             const uint8_t                * p_data,
                             bool                           cca,
                             bool                           immediate,
                             nrf_802154_notification_func_t notify_function,
                             bool                         * p_result)
{
    nrf_802154_req_data_t * p_slot = req_enter();

    p_slot->type                     = REQ_TYPE_TRANSMIT;
    p_slot->data.transmit.term_lvl   = term_lvl;
    p_slot->data.transmit.req_orig   = req_orig;
    p_slot->data.transmit.p_data     = p_data;
    p_slot->data.transmit.cca        = cca;
    p_slot->data.transmit.immediate  = immediate;
    p_slot->data.transmit.notif_func = notify_function;
    p_slot->data.transmit.p_result   = p_result;

    req_exit();
}

void nrf_802154_swi_energy_detection(nrf_802154_term_t term_lvl,
                                     uint32_t          time_us,
                                     bool            * p_result)
{
    nrf_802154_req_data_t * p_slot = req_enter();

    p_slot->type                           = REQ_TYPE_ENERGY_DETECTION;
    p_slot->data.energy_detection.term_lvl = term_lvl;
    p_slot->data.energy_detection.time_us  = time_us;
    p_slot->data.energy_detection.p_result = p_result;

    req_exit();
}

void nrf_802154_swi_cca(nrf_802154_term_t term_lvl, bool * p_result)
{
    nrf_802154_req_data_t * p_slot = req_enter();

    p_slot->type              = REQ_TYPE_CCA;
    p_slot->data.cca.term_lvl = term_lvl;
    p_slot->data.cca.p_result = p_result;

    req_exit();
}

void nrf_802154_swi_continuous_carrier(nrf_802154_term_t term_lvl, bool * p_result)
{
    nrf_802154_req_data_t * p_slot = req_enter();

    p_slot->type                             = REQ_TYPE_CONTINUOUS_CARRIER;
    p_slot->data.continuous_carrier.term_lvl = term_lvl;
    p_slot->data.continuous_carrier.p_result = p_result;

    req_exit();
}

void nrf_802154_swi_buffer_free(uint8_t * p_data, bool * p_result)
{
    nrf_802154_req_data_t * p_slot = req_enter();

    p_slot->type                      = REQ_TYPE_BUFFER_FREE;
    p_slot->data.buffer_free.p_data   = p_data;
    p_slot->data.buffer_free.p_result = p_result;

    req_exit();
}

void nrf_802154_swi_channel_update(bool * p_result)
{
    nrf_802154_req_data_t * p_slot = req_enter();

    p_slot->type                         = REQ_TYPE_CHANNEL_UPDATE;
    p_slot->data.channel_update.p_result = p_result;

    req_exit();
}

void nrf_802154_swi_cca_cfg_update(bool * p_result)
{
    nrf_802154_req_data_t * p_slot = req_enter();

    p_slot->type                         = REQ_TYPE_CCA_CFG_UPDATE;
    p_slot->data.cca_cfg_update.p_result = p_result;

    req_exit();
}

void SWI_IRQHandler(void)
{
    if (nrf_egu_event_check(SWI_EGU, NTF_EVENT))
    {
        nrf_egu_event_clear(SWI_EGU, NTF_EVENT);

        while (!ntf_queue_is_empty())
        {
            nrf_802154_ntf_data_t * p_slot = &m_ntf_queue[m_ntf_r_ptr];

            switch (p_slot->type)
            {
                case NTF_TYPE_RECEIVED:
#if NRF_802154_USE_RAW_API
                    nrf_802154_received_raw(p_slot->data.received.p_psdu,
                                            p_slot->data.received.power,
                                            p_slot->data.received.lqi);
#else // NRF_802154_USE_RAW_API
                    nrf_802154_received(p_slot->data.received.p_psdu + RAW_PAYLOAD_OFFSET,
                                        p_slot->data.received.p_psdu[RAW_LENGTH_OFFSET],
                                        p_slot->data.received.power,
                                        p_slot->data.received.lqi);
#endif
                    break;

                case NTF_TYPE_RECEIVE_FAILED:
                    nrf_802154_receive_failed(p_slot->data.receive_failed.error);
                    break;

                case NTF_TYPE_TRANSMITTED:
#if NRF_802154_USE_RAW_API
                    nrf_802154_transmitted_raw(p_slot->data.transmitted.p_frame,
                                               p_slot->data.transmitted.p_psdu,
                                               p_slot->data.transmitted.power,
                                               p_slot->data.transmitted.lqi);
#else // NRF_802154_USE_RAW_API
                    nrf_802154_transmitted(p_slot->data.transmitted.p_frame + RAW_PAYLOAD_OFFSET,
                                           p_slot->data.transmitted.p_psdu == NULL ? NULL :
                                           p_slot->data.transmitted.p_psdu + RAW_PAYLOAD_OFFSET,
                                           p_slot->data.transmitted.p_psdu[RAW_LENGTH_OFFSET],
                                           p_slot->data.transmitted.power,
                                           p_slot->data.transmitted.lqi);
#endif
                    break;

                case NTF_TYPE_TRANSMIT_FAILED:
#if NRF_802154_USE_RAW_API
                    nrf_802154_transmit_failed(p_slot->data.transmit_failed.p_frame,
                                               p_slot->data.transmit_failed.error);
#else // NRF_802154_USE_RAW_API
                    nrf_802154_transmit_failed(
                        p_slot->data.transmit_failed.p_frame + RAW_PAYLOAD_OFFSET,
                        p_slot->data.transmit_failed.error);
#endif
                    break;

                case NTF_TYPE_ENERGY_DETECTED:
                    nrf_802154_energy_detected(p_slot->data.energy_detected.result);
                    break;

                case NTF_TYPE_ENERGY_DETECTION_FAILED:
                    nrf_802154_energy_detection_failed(
                        p_slot->data.energy_detection_failed.error);
                    break;

                case NTF_TYPE_CCA:
                    nrf_802154_cca_done(p_slot->data.cca.result);
                    break;

                case NTF_TYPE_CCA_FAILED:
                    nrf_802154_cca_failed(p_slot->data.cca_failed.error);
                    break;

                default:
                    assert(false);
            }

            ntf_queue_ptr_increment(&m_ntf_r_ptr);
        }
    }

    if (nrf_egu_event_check(SWI_EGU, HFCLK_STOP_EVENT))
    {
        nrf_802154_clock_hfclk_stop();

        nrf_egu_event_clear(SWI_EGU, HFCLK_STOP_EVENT);
    }

    if (nrf_egu_event_check(SWI_EGU, REQ_EVENT))
    {
        nrf_egu_event_clear(SWI_EGU, REQ_EVENT);

        while (!req_queue_is_empty())
        {
            nrf_802154_req_data_t * p_slot = &m_req_queue[m_req_r_ptr];

            switch (p_slot->type)
            {
                case REQ_TYPE_SLEEP:
                    *(p_slot->data.sleep.p_result) =
                        nrf_802154_core_sleep(p_slot->data.sleep.term_lvl);
                    break;

                case REQ_TYPE_RECEIVE:
                    *(p_slot->data.receive.p_result) =
                        nrf_802154_core_receive(p_slot->data.receive.term_lvl,
                                                p_slot->data.receive.req_orig,
                                                p_slot->data.receive.notif_func,
                                                p_slot->data.receive.notif_abort);
                    break;

                case REQ_TYPE_TRANSMIT:
                    *(p_slot->data.transmit.p_result) =
                        nrf_802154_core_transmit(p_slot->data.transmit.term_lvl,
                                                 p_slot->data.transmit.req_orig,
                                                 p_slot->data.transmit.p_data,
                                                 p_slot->data.transmit.cca,
                                                 p_slot->data.transmit.immediate,
                                                 p_slot->data.transmit.notif_func);
                    break;

                case REQ_TYPE_ENERGY_DETECTION:
                    *(p_slot->data.energy_detection.p_result) =
                        nrf_802154_core_energy_detection(
                            p_slot->data.energy_detection.term_lvl,
                            p_slot->data.energy_detection.time_us);
                    break;

                case REQ_TYPE_CCA:
                    *(p_slot->data.cca.p_result) = nrf_802154_core_cca(p_slot->data.cca.term_lvl);
                    break;

                case REQ_TYPE_CONTINUOUS_CARRIER:
                    *(p_slot->data.continuous_carrier.p_result) =
                        nrf_802154_core_continuous_carrier(
                            p_slot->data.continuous_carrier.term_lvl);
                    break;

                case REQ_TYPE_BUFFER_FREE:
                    *(p_slot->data.buffer_free.p_result) =
                        nrf_802154_core_notify_buffer_free(p_slot->data.buffer_free.p_data);
                    break;

                case REQ_TYPE_CHANNEL_UPDATE:
                    *(p_slot->data.channel_update.p_result) = nrf_802154_core_channel_update();
                    break;

                case REQ_TYPE_CCA_CFG_UPDATE:
                    *(p_slot->data.cca_cfg_update.p_result) = nrf_802154_core_cca_cfg_update();
                    break;

                default:
                    assert(false);
            }

            req_queue_ptr_increment(&m_req_r_ptr);
        }
    }
}
