/* Copyright (c) 2016, Nordic Semiconductor ASA
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
 *   This file implements the nrf 802.15.4 radio driver.
 *
 */

#include "nrf_drv_radio802154.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <nrf.h>
#include <nrf_peripherals.h>
#include <hal/nrf_radio.h>

#if RADIO_RX_BUFFERS < 1
#error Not enough rx buffers in the 802.15.4 radio driver.
#endif

#define ACK_HEADER_WITH_PENDING      0x12  // First byte of ACK frame containing pending bit
#define ACK_HEADER_WITHOUT_PENDING   0x02  // First byte of ACK frame without pending bit

#define ACK_LENGTH                   5         // Length of ACK frame
#define ACK_REQUEST_BIT              (1 << 5)  // Ack request bit
#define ACK_REQUEST_OFFSET           1         // Byte containing Ack request bit (+1 for frame length byte)
#define DEST_ADDR_TYPE_EXTENDED      0x0c      // Bits containing extended destination address type
#define DEST_ADDR_TYPE_MASK          0x0c      // Mask of bits containing destination address type
#define DEST_ADDR_TYPE_OFFSET        2         // Byte containing destination address type (+1 for frame length byte)
#define DEST_ADDR_TYPE_SHORT         0x08      // Bits containing short destination address type
#define DEST_ADDR_OFFSET             5         // Offset of destination address in Data frame
#define DSN_OFFSET                   3         // Byte containing DSN value (+1 for frame length byte)
#define FRAME_PENDING_BIT            (1 << 4)  // Pending bit
#define FRAME_PENDING_OFFSET         1         // Byte containing pending bit (+1 for frame length byte)
#define FRAME_TYPE_ACK               0x02      // Bits containing ACK frame type
#define FRAME_TYPE_BEACON            0x00      // Bits containing Beacon frame type
#define FRAME_TYPE_COMMAND           0x03      // Bits containing Command frame type
#define FRAME_TYPE_DATA              0x01      // Bits containing Data frame type
#define FRAME_TYPE_MASK              0x07      // Mask of bits containing frame type
#define FRAME_TYPE_OFFSET            1         // Byte containing frame type bits (+1 for frame length byte)
#define PAN_ID_COMPR_OFFSET          1         // Byte containing Pan Id compression bit (+1 for frame length byte)
#define PAN_ID_COMPR_MASK            0x40      // Pan Id compression bit
#define PAN_ID_OFFSET                3         // Offset of Pan Id in Data frame
#define SRC_ADDR_TYPE_EXTENDED       0xc0      // Bits containing extended source address type
#define SRC_ADDR_TYPE_MASK           0xc0      // Mask of bits containing source address type
#define SRC_ADDR_TYPE_OFFSET         2         // Byte containing source address type (+1 for frame length byte)
#define SRC_ADDR_TYPE_SHORT          0x80      // Bits containing short source address type
#define SRC_ADDR_OFFSET_SHORT_DST    8         // Offset of source address in Data frame if destination address is short
#define SRC_ADDR_OFFSET_EXTENDED_DST 14        // Offset of source address in Data frame if destination address is extended

#define CRC_LENGTH      2         // Length of CRC in 802.15.4 frames [bytes]
#define CRC_POLYNOMIAL  0x011021  // Polynomial used for CRC calculation in 802.15.4 frames

#define MHMU_MASK               0xff0007ff // Mask of known bytes in ACK packet
#define MHMU_PATTERN            0x00000205 // Values of known bytes in ACK packet
#define MHMU_PATTERN_DSN_OFFSET 24         // Offset of DSN in MHMU_PATTER [bits]

#define PAN_ID_SIZE           2    // Size of Pan Id
#define SHORT_ADDRESS_SIZE    2    // Size of Short Mac Address
#define EXTENDED_ADDRESS_SIZE 8    // Size of Extended Mac Address
#define MAX_PACKET_SIZE       127  // Maximal size of radio packet

#define BROADCAST_ADDRESS    ((uint8_t [SHORT_ADDRESS_SIZE]) {0xff, 0xff}) // Broadcast Short Address

// Maximum number of Short Addresses of nodes for which there is pending data in buffer.
#define NUM_PENDING_SHORT_ADDRESSES     RADIO_PENDING_SHORT_ADDRESSES
// Maximum number of Extended Addresses of nodes for which there is pending data in buffer.
#define NUM_PENDING_EXTENDED_ADDRESSES  RADIO_PENDING_EXTENDED_ADDRESSES
// Value used to mark Short Address as unused.
#define UNUSED_PENDING_SHORT_ADDRESS    ((uint8_t [SHORT_ADDRESS_SIZE]) {0xff, 0xff})
// Value used to mark Extended Address as unused.
#define UNUSED_PENDING_EXTENDED_ADDRESS ((uint8_t [EXTENDED_ADDRESS_SIZE]) {0})

// Delay before sending ACK (10sym = 192uS)
#define TIFS_ACK_US         192
// Delay before first check of received frame: 16 bits is MAC Frame Control field.
#define BCC_INIT            (2 * 8)
// Delay before second check of received frame if destination address is short.
#define BCC_SHORT_ADDR      ((DEST_ADDR_OFFSET + SHORT_ADDRESS_SIZE) * 8)
// Delay before second check of received frame if destination address is extended.
#define BCC_EXTENDED_ADDR   ((DEST_ADDR_OFFSET + EXTENDED_ADDRESS_SIZE) * 8)

// Get LQI of given received packet. If CRC is calculated by hardware LQI is included instead of CRC
// in the frame. Length is stored in byte with index 0; CRC is 2 last bytes.
#define RX_FRAME_LQI(psdu)  ((psdu)[(psdu)[0] - 1])

#define SHORT_CCAIDLE_TXEN      1                         // Enable short between CCA Idle and TX Enable states.

typedef struct
{
    uint8_t psdu[MAX_PACKET_SIZE + 1];
    bool    free;                      // If this buffer is free or contains a frame.
} rx_buffer_t;

// Receive buffer
static rx_buffer_t m_receive_buffers[RADIO_RX_BUFFERS];

#if RADIO_RX_BUFFERS > 1
static rx_buffer_t * mp_current_rx_buffer; // Pointer to currently used receive buffer.
#else
static rx_buffer_t * const mp_current_rx_buffer = &m_receive_buffers[0]; // If there is only one buffer use const pointer.
#endif

// Ack frame buffer
static uint8_t m_ack_psdu[ACK_LENGTH + 1];

// Radio driver states
typedef enum
{
    // Sleep
    RADIO_STATE_SLEEP,             // Low power (DISABLED) mode

    // Receive
    RADIO_STATE_WAITING_RX_FRAME,  // Waiting for frame in receiver mode
    RADIO_STATE_RX_HEADER,         // Received SFD, receiving MAC header
    RADIO_STATE_RX_FRAME,          // Received MAC destination address, receiving rest of the frame
    RADIO_STATE_TX_ACK,            // Received frame and transmitting ACK

    // Transmit
    RADIO_STATE_CCA,               // Performing CCA
    RADIO_STATE_TX_FRAME,          // Transmitting data frame (or beacon)
    RADIO_STATE_RX_ACK,            // Receiving ACK after transmitted frame

    // Energy Detection
    RADIO_STATE_ED,                // Performing Energy Detection procedure
} radio_state_t;

static          bool          m_promiscuous = false;              // Indicating if radio is in promiscuous mode
static volatile radio_state_t m_state       = RADIO_STATE_SLEEP;  // State of the radio driver

static uint8_t m_pan_id[PAN_ID_SIZE]            = {0xff, 0xff};  // Pan Id of this node
static uint8_t m_short_addr[SHORT_ADDRESS_SIZE] = {0xfe, 0xff};  // Short Address of this node
static uint8_t m_extended_addr[EXTENDED_ADDRESS_SIZE];           // Extended Address of this node

typedef struct
{
    bool prevent_ack :1;  // If frame being received is not destined to this node (promiscuous mode).
} nrf_radio802154_flags_t;
static nrf_radio802154_flags_t m_flags;

// Mutex preventing race condition.
static volatile uint8_t m_mutex;

// If pending bit in ACK frame should be set to valid or default value.
static bool m_setting_pending_bit_enabled = true;
// Array of Short Addresses of nodes for which there is pending data in the buffer.
static uint8_t m_pending_short[NUM_PENDING_SHORT_ADDRESSES][SHORT_ADDRESS_SIZE];
// Array of Extended Addresses of nodes for which there is pending data in the buffer.
static uint8_t m_pending_extended[NUM_PENDING_EXTENDED_ADDRESSES][EXTENDED_ADDRESS_SIZE];

static const uint8_t * mp_tx_data;    // Pointer to data to transmit.

// Lock mutex to prevent run conditions.
static bool mutex_lock(void)
{
    do
    {
        volatile uint8_t mutex_value = __LDREXB(&m_mutex);

        if (mutex_value)
        {
            return false;
        }
    }
    while (__STREXB(1, &m_mutex));

    __DMB();

    assert(m_state == RADIO_STATE_WAITING_RX_FRAME);

    return true;
}

// Unlock mutex.
static void mutex_unlock(void)
{
    switch (m_state)
    {
    case RADIO_STATE_SLEEP:
    case RADIO_STATE_WAITING_RX_FRAME:
        break;

    default:
        assert(false);
    }

    __DMB();
    m_mutex = 0;
}

// Enter driver critical section.
static void critical_section_enter(void)
{
    NVIC_DisableIRQ(RADIO_IRQn);
    __DSB();
    __ISB();
}

// Exit driver critical section.
static void critical_section_exit(void)
{
    NVIC_EnableIRQ(RADIO_IRQn);
}


// Set radio state
static inline void state_set(radio_state_t state)
{
    m_state = state;
}

// Initialize static data.
static void data_init(void)
{
    memset(m_pending_extended, 0, sizeof(m_pending_extended));
    memset(m_pending_short, 0xff, sizeof(m_pending_short));

    const uint8_t ack_psdu[] = {0x05, ACK_HEADER_WITH_PENDING, 0x00, 0x00, 0x00, 0x00};
    memcpy(m_ack_psdu, ack_psdu, sizeof(ack_psdu));

    for (uint32_t i = 0; i < RADIO_RX_BUFFERS; i++)
    {
        m_receive_buffers[i].free = true;
    }
}

// Initialize radio peripheral and interrupts.
static void nrf_radio_init(void)
{
    nrf_radio_mode_set(NRF_RADIO_MODE_IEEE802154_250KBIT);

    const nrf_radio_packet_conf_t packet_conf =
    {
        .lflen  = 8,
        .crcinc = true,
        .plen   = NRF_RADIO_PREAMBLE_LENGTH_32BIT_ZERO,
        .maxlen = MAX_PACKET_SIZE,
    };
    nrf_radio_packet_configure(&packet_conf);

    // Configure CRC
    nrf_radio_crc_configure(CRC_LENGTH,
                            NRF_RADIO_CRC_ADDR_IEEE802154,
                            CRC_POLYNOMIAL);

    // Configure CCA
    nrf_radio_cca_configure(RADIO_CCA_MODE,
                            RADIO_CCA_ED_THRESHOLD,
                            RADIO_CCA_CORR_THRESHOLD,
                            RADIO_CCA_CORR_LIMIT);

    // Configure MAC Header Match Unit
    nrf_radio_mhmu_search_pattern_set(0);
    nrf_radio_mhmu_pattern_mask_set(MHMU_MASK);

    nrf_radio_int_enable(NRF_RADIO_INT_FRAMESTART_MASK |
                         NRF_RADIO_INT_END_MASK |
                         NRF_RADIO_INT_DISABLED_MASK |
#if !SHORT_CCAIDLE_TXEN
                         NRF_RADIO_INT_CCAIDLE_MASK |
#endif
                         NRF_RADIO_INT_CCABUSY_MASK |
                         NRF_RADIO_INT_READY_MASK |
                         NRF_RADIO_INT_BCMATCH_MASK |
                         NRF_RADIO_INT_EDEND_MASK);
#if SHORT_CCAIDLE_TXEN
    nrf_radio_shorts_enable(NRF_RADIO_SHORT_CCAIDLE_TXEN_MASK);
#endif
}

#ifdef RADIO_IRQ_CTRL
static void irq_init(void)
{
    NVIC_SetPriority(RADIO_IRQn, RADIO_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);
}
#endif

// Set radio channel
static void channel_set(uint8_t channel)
{
    nrf_radio_frequency_set(2405 + (5 * (channel - 11)));
}

// Get radio channel
static uint8_t channel_get(void)
{
    return ((nrf_radio_frequency_get() - 2405) / 5) + 11;
}

// Set transmit power
static void tx_power_set(int8_t dbm)
{
    const int8_t allowed_values[] =
    {
        NRF_RADIO_TXPOWER_NEG40DBM,
        NRF_RADIO_TXPOWER_NEG20DBM,
        NRF_RADIO_TXPOWER_NEG16DBM,
        NRF_RADIO_TXPOWER_NEG12DBM,
        NRF_RADIO_TXPOWER_NEG8DBM,
        NRF_RADIO_TXPOWER_NEG4DBM,
        NRF_RADIO_TXPOWER_0DBM,
        NRF_RADIO_TXPOWER_POS2DBM,
        NRF_RADIO_TXPOWER_POS3DBM,
        NRF_RADIO_TXPOWER_POS4DBM,
        NRF_RADIO_TXPOWER_POS5DBM,
        NRF_RADIO_TXPOWER_POS6DBM,
        NRF_RADIO_TXPOWER_POS7DBM,
        NRF_RADIO_TXPOWER_POS8DBM,
    };
    const size_t value_count =
        sizeof(allowed_values) / sizeof(allowed_values[0]);
    const int8_t highest_value = (int8_t)allowed_values[value_count - 1];
    if (dbm > highest_value)
    {
        dbm = highest_value;
    }
    else
    {
        for (uint32_t i = 0; i < value_count; i++)
        {
            if (dbm <= allowed_values[i])
            {
                dbm = allowed_values[i];
                break;
            }
        }
    }

    nrf_radio_txpower_set(dbm);
}

static inline void rx_enable(void)
{
    nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
}

static inline void rx_start(void)
{
    nrf_radio_packetptr_set(mp_current_rx_buffer->psdu);
    nrf_radio_task_trigger(NRF_RADIO_TASK_START);

    // Just after starting receiving to receive buffer set packet pointer to ACK frame that can be
    // sent automatically.
    nrf_radio_packetptr_set(m_ack_psdu);
}

static void received_frame_notify(void)
{
    mp_current_rx_buffer->free = false;
    nrf_drv_radio802154_received(mp_current_rx_buffer->psdu,                // data
                                 nrf_drv_radio802154_rssi_last_get(),       // rssi
                                 RX_FRAME_LQI(mp_current_rx_buffer->psdu)); // lqi
}

static void rx_buffer_in_use_set(rx_buffer_t * p_rx_buffer)
{
#if RADIO_RX_BUFFERS > 1
    mp_current_rx_buffer = p_rx_buffer;
#else
    (void) p_rx_buffer;
#endif
}

static rx_buffer_t * free_rx_buffer_find(void)
{
    for (uint32_t i = 0; i < RADIO_RX_BUFFERS; i++)
    {
        if (m_receive_buffers[i].free)
        {
            return &m_receive_buffers[i];
        }
    }

    return NULL;
}

// Set valid sequence number in ACK frame.
static inline void ack_prepare(void)
{
    // Copy sequence number from received frame to ACK frame.
    m_ack_psdu[DSN_OFFSET] = mp_current_rx_buffer->psdu[DSN_OFFSET];
}

// Set pending bit in ACK frame.
static inline void ack_pending_bit_set(void)
{
    uint8_t * p_src_addr;
    uint32_t  i;
    bool      found = false;

    m_ack_psdu[FRAME_PENDING_OFFSET] = ACK_HEADER_WITH_PENDING;

    if (!m_setting_pending_bit_enabled)
    {
        return;
    }

    switch (mp_current_rx_buffer->psdu[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK)
    {
    case DEST_ADDR_TYPE_SHORT:
        p_src_addr = &mp_current_rx_buffer->psdu[SRC_ADDR_OFFSET_SHORT_DST];
        break;

    case DEST_ADDR_TYPE_EXTENDED:
        p_src_addr = &mp_current_rx_buffer->psdu[SRC_ADDR_OFFSET_EXTENDED_DST];
        break;

    default:
        return;
    }

    if (0 == (mp_current_rx_buffer->psdu[PAN_ID_COMPR_OFFSET] & PAN_ID_COMPR_MASK))
    {
        p_src_addr += 2;
    }

    switch (mp_current_rx_buffer->psdu[SRC_ADDR_TYPE_OFFSET] & SRC_ADDR_TYPE_MASK)
    {
    case SRC_ADDR_TYPE_SHORT:
        for (i = 0; i < NUM_PENDING_SHORT_ADDRESSES; i++)
        {
            if (nrf_radio_state_get() != NRF_RADIO_STATE_TXRU)
            {
                break;
            }

            if (0 == memcmp(p_src_addr, m_pending_short[i], sizeof(m_pending_short[i])))
            {
                found = true;
                break;
            }
        }

        break;

    case SRC_ADDR_TYPE_EXTENDED:
        for (i = 0; i < NUM_PENDING_EXTENDED_ADDRESSES; i++)
        {
            if (nrf_radio_state_get() != NRF_RADIO_STATE_TXRU)
            {
                break;
            }

            if (0 == memcmp(p_src_addr, m_pending_extended[i], sizeof(m_pending_extended[i])))
            {
                found = true;
                break;
            }
        }

        break;

    default:
        return;
    }

    if (!found)
    {
        m_ack_psdu[FRAME_PENDING_OFFSET] = ACK_HEADER_WITHOUT_PENDING;
    }
}

// Check if ACK is requested in given frame.
static inline bool ack_is_requested(const uint8_t * p_frame)
{
    return (p_frame[ACK_REQUEST_OFFSET] & ACK_REQUEST_BIT) ? true : false;
}

// Check if received destination address matches local or broadcast address.
static inline bool received_dest_addr_matched(void)
{
    // Check destination PAN Id.
    // Note that +1 in PSDU offset is added because first byte in PSDU is length.
    if ((0 != memcmp(&mp_current_rx_buffer->psdu[PAN_ID_OFFSET + 1], m_pan_id, PAN_ID_SIZE)) &&
        (0 != memcmp(&mp_current_rx_buffer->psdu[PAN_ID_OFFSET + 1], BROADCAST_ADDRESS, PAN_ID_SIZE)))
    {
        return false;
    }

    // Check destination address.
    switch (mp_current_rx_buffer->psdu[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK)
    {
    case DEST_ADDR_TYPE_SHORT:
    {
        // Note that +1 in PSDU offset is added because first byte in PSDU is length.
        if ((0 != memcmp(&mp_current_rx_buffer->psdu[DEST_ADDR_OFFSET + 1], m_short_addr, SHORT_ADDRESS_SIZE)) &&
            (0 != memcmp(&mp_current_rx_buffer->psdu[DEST_ADDR_OFFSET + 1], BROADCAST_ADDRESS, SHORT_ADDRESS_SIZE)))
        {
            return false;
        }

        break;
    }

    case DEST_ADDR_TYPE_EXTENDED:
    {
        // Note that +1 in PSDU offset is added because first byte in PSDU is length.
        if (0 != memcmp(&mp_current_rx_buffer->psdu[DEST_ADDR_OFFSET + 1], m_extended_addr, sizeof(m_extended_addr)))
        {
            return false;
        }

        break;
    }

    default:
        return false;
    }

    return true;
}

// Get result of last RSSI measurement.
static inline int8_t last_ed_result_get(void)
{
    // TODO: Change this conversion to correct one after lab tests.
    return nrf_radio_ed_sample_get();
}

// Enable peripheral shorts used during data frame transmission.
static inline void shorts_tx_frame_enable(void)
{
    nrf_radio_shorts_enable(NRF_RADIO_SHORT_END_DISABLE_MASK | NRF_RADIO_SHORT_READY_START_MASK);
}

// Disable peripheral shorts used during data frame transmission.
static inline void shorts_tx_frame_disable(void)
{
    nrf_radio_shorts_disable(NRF_RADIO_SHORT_END_DISABLE_MASK | NRF_RADIO_SHORT_READY_START_MASK);
}

// Enable peripheral shorts used during automatic ACK transmission.
// TIFS shorts are splitted
static inline void shorts_tifs_initial_enable(void)
{
    nrf_radio_ifs_set(TIFS_ACK_US);
    nrf_radio_bcc_set(BCC_INIT);

    nrf_radio_shorts_enable(NRF_RADIO_SHORT_END_DISABLE_MASK | NRF_RADIO_SHORT_DISABLED_TXEN_MASK |
                            NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK);
}

static inline void shorts_tifs_following_enable(void)
{
    nrf_radio_shorts_disable(NRF_RADIO_SHORT_DISABLED_TXEN_MASK);
    nrf_radio_shorts_enable(NRF_RADIO_SHORT_READY_START_MASK);
}

// Disable peripheral shorts used during automatic ACK transmission if ACK is transmitted
// (short disabling transmitter persists).
static inline void shorts_tifs_ack_disable(void)
{
    // If ACK is sent END_DISABLE short should persist to disable transmitter automatically.
    nrf_radio_shorts_disable(NRF_RADIO_SHORT_DISABLED_TXEN_MASK |
                             NRF_RADIO_SHORT_READY_START_MASK | NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK);
    nrf_radio_ifs_set(0);
}

// Disable peripheral shorts used during automatic ACK transmission if ACK is not transmitted
// (all shorts are disabled).
static inline void shorts_tifs_no_ack_disable(void)
{
    nrf_radio_shorts_disable(NRF_RADIO_SHORT_END_DISABLE_MASK | NRF_RADIO_SHORT_DISABLED_TXEN_MASK |
                             NRF_RADIO_SHORT_READY_START_MASK | NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK);
    nrf_radio_ifs_set(0);
}

// Assert that peripheral shorts used during automatic ACK transmission are enabled.
static inline void assert_tifs_shorts_enabled(void)
{
    assert(NRF_RADIO->SHORTS & NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK);
}

// Assert that peripheral shorts used during automatic ACK transmission are disabled.
static inline void assert_tifs_shorts_disabled(void)
{
    assert(!(NRF_RADIO->SHORTS & NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK));
}

// Abort automatic sending ACK.
static void auto_ack_abort(radio_state_t state_to_set)
{
    shorts_tifs_no_ack_disable();

    switch (nrf_radio_state_get())
    {
    case NRF_RADIO_STATE_RX:     // When stopping before whole frame received.
    case NRF_RADIO_STATE_RXRU:  // When transmission is initialized during receiver ramp up.
    case NRF_RADIO_STATE_RXIDLE:
    case NRF_RADIO_STATE_TXRU:
    case NRF_RADIO_STATE_TXIDLE:
    case NRF_RADIO_STATE_TX:
        nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED); // Clear disabled event that was set by short.
        state_set(state_to_set);
        nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
        break;

    case NRF_RADIO_STATE_RXDISABLE:
    case NRF_RADIO_STATE_DISABLED:
        // Do not trigger DISABLE task in those states to prevent double DISABLED events.
        state_set(state_to_set);
        break;

    default:
        assert(false);
    }
}

static inline bool tx_procedure_begin(const uint8_t * p_data, uint8_t channel, int8_t power)
{
    bool result = false;

    critical_section_enter();

    if (mutex_lock())
    {
        switch (m_state)
        {
        case RADIO_STATE_WAITING_RX_FRAME:
            channel_set(channel);
            auto_ack_abort(RADIO_STATE_CCA);

            assert_tifs_shorts_disabled();

            tx_power_set(power);
            nrf_radio_packetptr_set(p_data);

            // Clear events that could have happened in critical section due to receiving frame or RX ramp up.
            nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
            nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
            nrf_radio_event_clear(NRF_RADIO_EVENT_END);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

            result = true;

            break;

        default:
            assert(false); // This should not happen.
        }
    }

    critical_section_exit();

    return result;
}

static inline void tx_procedure_abort(void)
{
    critical_section_enter();

    switch (m_state)
    {
    case RADIO_STATE_CCA:
    case RADIO_STATE_TX_FRAME:
    case RADIO_STATE_RX_ACK:
        shorts_tx_frame_disable();

        assert_tifs_shorts_disabled();
        assert(m_mutex);

        state_set(RADIO_STATE_WAITING_RX_FRAME);

        switch (nrf_radio_state_get())
        {
        case NRF_RADIO_STATE_TXDISABLE:
        case NRF_RADIO_STATE_RXDISABLE:
            // Do not enabled receiver. It will be enabled in DISABLED handler.
            break;

        default:
            nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);
            rx_enable();
        }

        nrf_radio_mhmu_search_pattern_set(0);
        nrf_radio_event_clear(NRF_RADIO_EVENT_MHRMATCH);

        // Clear events that could have happened in critical section due to receiving frame.
        nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
        nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
        nrf_radio_event_clear(NRF_RADIO_EVENT_END);

        break;

    // Just before entering this event handler Ack Frame could be received and
    // driver enters WatiningRxFrame state. In this case just clear pending events
    // because change of state was requested.
    case RADIO_STATE_WAITING_RX_FRAME:
        break;

    default:
        assert(false);
    }

    critical_section_exit();
}

static inline void enabling_rx_procedure_begin(rx_buffer_t * p_buffer)
{
    assert(p_buffer->free == false);

    critical_section_enter();

    p_buffer->free = true;

    switch (m_state)
    {
    case RADIO_STATE_WAITING_RX_FRAME:

        switch (nrf_radio_state_get())
        {
        case NRF_RADIO_STATE_RXDISABLE: // This one could happen after receive of broadcast frame.
        case NRF_RADIO_STATE_TXDISABLE: // This one could happen due to stopping ACK.
        case NRF_RADIO_STATE_DISABLED:  // This one could happen during stopping ACK.
        case NRF_RADIO_STATE_RXRU:      // This one could happen during enabling receiver (after sending ACK).
        case NRF_RADIO_STATE_RX:        // This one could happen if any other buffer is in use.
            break;

        case NRF_RADIO_STATE_RXIDLE:
            // Mutex to make sure Radio State did not change between IRQ and this process.
            // If API call changed Radio state leave Radio as it is.
            if (mutex_lock())
            {
                shorts_tifs_initial_enable();

                rx_buffer_in_use_set(p_buffer);
                rx_start();

                // Clear events that could have happened in critical section due to RX ramp up.
                nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

                mutex_unlock();
            }
            break;

        default:
            assert(false);
        }

        break;

    default:
        // Don't perform any action in any other state (receiver should not be started).
        break;
    }

    critical_section_exit();
}

static inline bool energy_detection_procedure_begin(uint8_t tx_channel)
{
    bool result = false;

    critical_section_enter();

    if (mutex_lock())
    {
        switch (m_state)
        {
        case RADIO_STATE_WAITING_RX_FRAME:
            channel_set(tx_channel);
            auto_ack_abort(RADIO_STATE_ED);

            assert_tifs_shorts_disabled();

            // Clear events that could have happened in critical section due to receiving frame or RX ramp up.
            nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
            nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
            nrf_radio_event_clear(NRF_RADIO_EVENT_END);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

            result = true;

            break;

        default:
            assert(false); // This should not happen.
        }
    }

    critical_section_exit();

    return result;
}

static inline bool sleep_procedure_begin(void)
{
    bool result = false;

    critical_section_enter();

    if (mutex_lock())
    {
        switch (m_state)
        {
        case RADIO_STATE_WAITING_RX_FRAME:
            auto_ack_abort(RADIO_STATE_SLEEP);

            assert_tifs_shorts_disabled();

            // Clear events that could have happened in critical section due to receiving frame or RX ramp up.
            nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);
            nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);
            nrf_radio_event_clear(NRF_RADIO_EVENT_END);
            nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

            result = true;

            break;

        default:
            assert(false); // This should not happen.
        }
    }

    critical_section_exit();

    return result;
}

static void radio_reset(void)
{
    uint8_t channel = channel_get();

    nrf_radio_power_set(false);
    nrf_radio_power_set(true);

    nrf_radio_init();

    channel_set(channel);

    switch (m_state)
    {
    case RADIO_STATE_WAITING_RX_FRAME:
    case RADIO_STATE_RX_HEADER:
    case RADIO_STATE_RX_FRAME:
    case RADIO_STATE_TX_ACK:
        state_set(RADIO_STATE_WAITING_RX_FRAME);
        rx_enable();
        break;

    case RADIO_STATE_CCA:
    case RADIO_STATE_TX_FRAME:
    case RADIO_STATE_RX_ACK:
        nrf_drv_radio802154_busy_channel();
        state_set(RADIO_STATE_WAITING_RX_FRAME);
        rx_enable();
        break;

    case RADIO_STATE_ED:
        nrf_drv_radio802154_energy_detected(0);
        state_set(RADIO_STATE_WAITING_RX_FRAME);
        rx_enable();
        break;

    case RADIO_STATE_SLEEP:
        mutex_unlock();
        break;

    default:
        assert(false);
    }
}

uint8_t nrf_drv_radio802154_channel_get(void)
{
    return channel_get();
}

void nrf_drv_radio802154_pan_id_set(const uint8_t * p_pan_id)
{
    memcpy(m_pan_id, p_pan_id, PAN_ID_SIZE);
}

void nrf_drv_radio802154_extended_address_set(const uint8_t * p_extended_address)
{
    memcpy(m_extended_addr, p_extended_address, EXTENDED_ADDRESS_SIZE);
}

void nrf_drv_radio802154_short_address_set(const uint8_t * p_short_address)
{
    memcpy(m_short_addr, p_short_address, SHORT_ADDRESS_SIZE);
}

void nrf_drv_radio802154_init(void)
{
    data_init();

    nrf_radio_init();
#ifdef RADIO_IRQ_CTRL
    irq_init();
#endif
}

bool nrf_drv_radio802154_sleep(void)
{
    bool result = true;

    switch (m_state)
    {
    case RADIO_STATE_SLEEP:
        break;

    case RADIO_STATE_WAITING_RX_FRAME:
    case RADIO_STATE_RX_HEADER:
    case RADIO_STATE_RX_FRAME:
    case RADIO_STATE_TX_ACK:
        result = sleep_procedure_begin();
        break;

    default:
        assert(false);
    }

    return result;
}

bool nrf_drv_radio802154_receive(uint8_t channel, bool force_rx)
{
    bool result = true;

    switch (m_state)
    {
    case RADIO_STATE_WAITING_RX_FRAME:
    case RADIO_STATE_RX_HEADER:
    case RADIO_STATE_RX_FRAME:
    case RADIO_STATE_TX_ACK:
        if (channel_get() != channel)
        {
            if ((m_state != RADIO_STATE_WAITING_RX_FRAME) && !force_rx)
            {
                result = false;
            }
            else
            {
               channel_set(channel);
               if (mutex_lock())
               {
                   auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
               }
            }
        }
        break;

    case RADIO_STATE_SLEEP:
        if (nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED)
        {
            assert_tifs_shorts_disabled();

            state_set(RADIO_STATE_WAITING_RX_FRAME);

            channel_set(channel);

            rx_enable();
        }
        break;

    case RADIO_STATE_TX_FRAME:
    case RADIO_STATE_RX_ACK:
        if (force_rx)
        {
            tx_procedure_abort();
        }
        else
        {
            result = false;
        }
        break;

    default:
        assert(false);
    }

    return result;
}

bool nrf_drv_radio802154_transmit(const uint8_t * p_data, uint8_t channel, int8_t power)
{
    mp_tx_data = p_data;

    return tx_procedure_begin(p_data, channel, power);
}

void nrf_drv_radio802154_buffer_free(uint8_t * p_data)
{
    enabling_rx_procedure_begin((rx_buffer_t *)p_data);
}

int8_t nrf_drv_radio802154_rssi_last_get(void)
{
    uint8_t minusDBm = nrf_radio_rssi_sample_get();
    return - (int8_t)minusDBm;
}

bool nrf_drv_radio802154_promiscuous_get(void)
{
    return m_promiscuous;
}

void nrf_drv_radio802154_promiscuous_set(bool enabled)
{
    m_promiscuous = enabled;
}

void nrf_drv_radio802154_auto_pending_bit_set(bool enabled)
{
    m_setting_pending_bit_enabled = enabled;
}

bool nrf_drv_radio802154_pending_bit_for_addr_set(const uint8_t * p_addr, bool extended)
{
    if (extended)
    {
        for (uint32_t i = 0; i < NUM_PENDING_EXTENDED_ADDRESSES; i++)
        {
            if (0 == memcmp(m_pending_extended[i], p_addr, sizeof(m_pending_extended[i])))
            {
                return true;
            }

            if (0 == memcmp(m_pending_extended[i], UNUSED_PENDING_EXTENDED_ADDRESS, sizeof(m_pending_extended[i])))
            {
                memcpy(m_pending_extended[i], p_addr, sizeof(m_pending_extended[i]));
                return true;
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < NUM_PENDING_SHORT_ADDRESSES; i++)
        {
            if (0 == memcmp(m_pending_short[i], p_addr, sizeof(m_pending_short[i])))
            {
                return true;
            }

            if (0 == memcmp(m_pending_short[i], UNUSED_PENDING_SHORT_ADDRESS, sizeof(m_pending_short[i])))
            {
                memcpy(m_pending_short[i], p_addr, sizeof(m_pending_short[i]));
                return true;
            }
        }
    }

    return false;
}

bool nrf_drv_radio802154_pending_bit_for_addr_clear(const uint8_t * p_addr, bool extended)
{
    bool result = false;

    if (extended)
    {
        for (uint32_t i = 0; i < NUM_PENDING_EXTENDED_ADDRESSES; i++)
        {
            if (0 == memcmp(m_pending_extended[i], p_addr, sizeof(m_pending_extended[i])))
            {
                memset(m_pending_extended[i], 0, sizeof(m_pending_extended[i]));
                result = true;
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < NUM_PENDING_SHORT_ADDRESSES; i++)
        {
            if (0 == memcmp(m_pending_short[i], p_addr, sizeof(m_pending_short[i])))
            {
                memset(m_pending_short[i], 0xff, sizeof(m_pending_short[i]));
                result = true;
            }
        }
    }

    return result;
}

void nrf_drv_radio802154_pending_bit_for_addr_reset(bool extended)
{
    if (extended)
    {
        memset(m_pending_extended, 0, sizeof(m_pending_extended));
    }
    else
    {
        memset(m_pending_short, 0xff, sizeof(m_pending_short));
    }
}

bool nrf_drv_radio802154_energy_detection(uint8_t channel, uint32_t time_us)
{

    nrf_radio_ed_loop_count_set(time_us / 128UL); // multiple of 10 symbols (128 us)

    return energy_detection_procedure_begin(channel);
}

void nrf_drv_radio802154_irq_handler(void)
{
    if (nrf_radio_event_check(NRF_RADIO_EVENT_FRAMESTART))
    {
        nrf_radio_event_clear(NRF_RADIO_EVENT_FRAMESTART);

        switch (m_state)
        {
        case RADIO_STATE_WAITING_RX_FRAME:
            if (mutex_lock())
            {
                state_set(RADIO_STATE_RX_HEADER);
                assert_tifs_shorts_enabled();

                if ((mp_current_rx_buffer->psdu[0] < ACK_LENGTH) ||
                    (mp_current_rx_buffer->psdu[0] > MAX_PACKET_SIZE))
                {
                    auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                }
                else
                {
                    nrf_radio_task_trigger(NRF_RADIO_TASK_RSSISTART);
                }
            }

            switch (nrf_radio_state_get())
            {
            case NRF_RADIO_STATE_RX:

            // If the received frame was short the radio could have changed it's state.
            case NRF_RADIO_STATE_RXIDLE:

            // The radio could have changed state to one of the following due to enabled shorts.
            case NRF_RADIO_STATE_RXDISABLE:
            case NRF_RADIO_STATE_DISABLED:
            case NRF_RADIO_STATE_TXRU:
                break;

            // If something had stopped the CPU too long. Try to recover radio state.
            case NRF_RADIO_STATE_TXIDLE:
            case NRF_RADIO_STATE_TXDISABLE:
                radio_reset();
                break;

            default:
                assert(false);
            }
            break;

        case RADIO_STATE_TX_ACK:
        case RADIO_STATE_TX_FRAME:
        case RADIO_STATE_RX_ACK:
        case RADIO_STATE_CCA: // This could happen at the beginning of transmission procedure.
            break;

        default:
            assert(false);
        }
    }


    // Check MAC frame header.
    if (nrf_radio_event_check(NRF_RADIO_EVENT_BCMATCH))
    {
        nrf_radio_event_clear(NRF_RADIO_EVENT_BCMATCH);

        switch (m_state)
        {
        case RADIO_STATE_RX_HEADER:
            assert_tifs_shorts_enabled();

            switch (nrf_radio_state_get())
            {
            case NRF_RADIO_STATE_RX:
            case NRF_RADIO_STATE_RXIDLE:
            case NRF_RADIO_STATE_RXDISABLE: // A lot of states due to shorts.
            case NRF_RADIO_STATE_DISABLED:
            case NRF_RADIO_STATE_TXRU:

                switch (nrf_radio_bcc_get())
                {
                case BCC_INIT:

                    // Check Frame Control field.
                    switch (mp_current_rx_buffer->psdu[FRAME_TYPE_OFFSET] & FRAME_TYPE_MASK)
                    {
                    case FRAME_TYPE_BEACON:
                        // Beacon is broadcast frame.
                        m_flags.prevent_ack = false;
                        state_set(RADIO_STATE_RX_FRAME);
                        break;

                    case FRAME_TYPE_DATA:
                    case FRAME_TYPE_COMMAND:

                        // For data or command check destination address.
                        switch (mp_current_rx_buffer->psdu[DEST_ADDR_TYPE_OFFSET] & DEST_ADDR_TYPE_MASK)
                        {
                        case DEST_ADDR_TYPE_SHORT:
                            nrf_radio_bcc_set(BCC_SHORT_ADDR);
                            break;

                        case DEST_ADDR_TYPE_EXTENDED:
                            nrf_radio_bcc_set(BCC_EXTENDED_ADDR);
                            break;

                        default:
                            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                        }

                        break;

                    default:

                        // For ACK and other types: in promiscous mode accept it as broadcast;
                        // in normal mode drop the frame.
                        if (m_promiscuous)
                        {
                            m_flags.prevent_ack = true;
                            state_set(RADIO_STATE_RX_FRAME);
                        }
                        else
                        {
                            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                        }
                    }

                    break;

                case BCC_SHORT_ADDR:
                case BCC_EXTENDED_ADDR:
                    // Check destination address during second match.
                    if (received_dest_addr_matched())
                    {
                        m_flags.prevent_ack = false;
                        state_set(RADIO_STATE_RX_FRAME);
                    }
                    else
                    {
                        if (m_promiscuous)
                        {
                            m_flags.prevent_ack = true;
                            state_set(RADIO_STATE_RX_FRAME);
                        }
                        else
                        {
                            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                        }
                    }

                    break;

                default:
                    assert(false);
                }

                break;

            case NRF_RADIO_STATE_TXIDLE:
                // Something had stopped the CPU too long. Try to recover radio state.
                radio_reset();
                break;

            default:
                assert(false);
            }

            break;

        default:
            assert(false);
        }
    }

    if (nrf_radio_event_check(NRF_RADIO_EVENT_END))
    {
        nrf_radio_event_clear(NRF_RADIO_EVENT_END);

        switch (m_state)
        {
        case RADIO_STATE_WAITING_RX_FRAME:

            // Radio state is not asserted here. It can be a lot of states due to shorts.
            if (mp_current_rx_buffer->psdu[0] == 0)
            {
                // If length of the frame is 0 there was no FRAMESTART event. Lock mutex now and abort sending ACK.
                if (mutex_lock())
                {
                    assert_tifs_shorts_enabled();
                    auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                }
            }
            else
            {
                // Do nothing. Whoever took mutex shall stop sending ACK.
            }

            break;

        case RADIO_STATE_RX_HEADER:
            // Frame ended before header was received.
            auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
            break;

        case RADIO_STATE_RX_FRAME:
            assert_tifs_shorts_enabled();

            switch (nrf_radio_state_get())
            {
            case NRF_RADIO_STATE_RXIDLE:
            case NRF_RADIO_STATE_RXDISABLE:
            case NRF_RADIO_STATE_DISABLED:
            case NRF_RADIO_STATE_TXRU:

                if (nrf_radio_crc_status_check())
                {
                    ack_prepare();

                    if ((!ack_is_requested(mp_current_rx_buffer->psdu)) || m_flags.prevent_ack)
                    {
                        auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                        received_frame_notify();
                    }
                    else
                    {
                        state_set(RADIO_STATE_TX_ACK);
                    }
                }
                else
                {
                    auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                }

                break;

            case NRF_RADIO_STATE_TXIDLE:
                // CPU was hold too long.
                nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
                auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
                break;

            default:
                assert(false);
            }

            break;

        case RADIO_STATE_TX_ACK: // Ended transmission of ACK.
            shorts_tifs_no_ack_disable();

            received_frame_notify();

            state_set(RADIO_STATE_WAITING_RX_FRAME);
            // Receiver is enabled by shorts.
            break;

        case RADIO_STATE_CCA: // This could happen at the beginning of transmission procedure (the procedure already has disabled shorts).
            assert_tifs_shorts_disabled();
            break;

        case RADIO_STATE_TX_FRAME:
            shorts_tx_frame_disable();
            assert_tifs_shorts_disabled();

            if (!ack_is_requested(mp_tx_data))
            {
                nrf_drv_radio802154_transmitted(false);

                state_set(RADIO_STATE_WAITING_RX_FRAME);
            }
            else
            {
                state_set(RADIO_STATE_RX_ACK);

                nrf_radio_event_clear(NRF_RADIO_EVENT_MHRMATCH);
                nrf_radio_mhmu_search_pattern_set(MHMU_PATTERN |
                                                  ((uint32_t) mp_tx_data[DSN_OFFSET] <<
                                                   MHMU_PATTERN_DSN_OFFSET));
            }

            // Task DISABLE is triggered by shorts.
            break;

        case RADIO_STATE_RX_ACK: // Ended receiving of ACK.
            assert_tifs_shorts_disabled();
            assert(nrf_radio_state_get() == NRF_RADIO_STATE_RXIDLE);

            if (nrf_radio_event_check(NRF_RADIO_EVENT_MHRMATCH) &&
                nrf_radio_crc_status_check())
            {
                nrf_drv_radio802154_transmitted(
                        (mp_current_rx_buffer->psdu[FRAME_PENDING_OFFSET] & FRAME_PENDING_BIT) != 0);

                nrf_radio_mhmu_search_pattern_set(0);
                nrf_radio_event_clear(NRF_RADIO_EVENT_MHRMATCH);
                state_set(RADIO_STATE_WAITING_RX_FRAME);
                shorts_tifs_initial_enable();
                rx_start();
                mutex_unlock();
            }
            else
            {
                nrf_radio_event_clear(NRF_RADIO_EVENT_MHRMATCH); // In case CRC is invalid.
                rx_start();
            }

            break;

        default:
            assert(false);
        }
    }

    if (nrf_radio_event_check(NRF_RADIO_EVENT_DISABLED))
    {
        nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);

        switch (m_state)
        {
        case RADIO_STATE_SLEEP:
            assert_tifs_shorts_disabled();
            assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);

            mutex_unlock();
            break;

        case RADIO_STATE_WAITING_RX_FRAME:
            assert_tifs_shorts_disabled();

            while (nrf_radio_state_get() == NRF_RADIO_STATE_TXDISABLE)
            {
                // This event can be handled in TXDISABLE state due to double DISABLE event (IC-15879).
                // This busy loop waits to the end of this state.
            }

            assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);

            nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);
            mutex_unlock();

            rx_buffer_in_use_set(free_rx_buffer_find());

            // Clear this event after RXEN task in case event is triggered just before.
            nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);
            break;

        case RADIO_STATE_TX_ACK:
            assert_tifs_shorts_enabled();

            shorts_tifs_following_enable();
            ack_pending_bit_set();

            if (nrf_radio_state_get() == NRF_RADIO_STATE_TXIDLE)
            {
                // CPU was hold too long.
                nrf_radio_event_clear(NRF_RADIO_EVENT_READY);
                auto_ack_abort(RADIO_STATE_WAITING_RX_FRAME);
            }
            break;

        case RADIO_STATE_CCA:
            assert_tifs_shorts_disabled();
            assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);
            nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);
            break;

        case RADIO_STATE_TX_FRAME:
            assert_tifs_shorts_disabled();
#if !SHORT_CCAIDLE_TXEN
            nrf_radio_task_trigger(NRF_RADIO_TASK_TXEN);
#endif
            break;

        case RADIO_STATE_RX_ACK:
            assert_tifs_shorts_disabled();
            assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);
            nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);
            break;

        case RADIO_STATE_ED:
            assert_tifs_shorts_disabled();
            assert(nrf_radio_state_get() == NRF_RADIO_STATE_DISABLED);
            nrf_radio_task_trigger(NRF_RADIO_TASK_RXEN);
            break;

        default:
            assert(false);
        }
    }

    if (nrf_radio_event_check(NRF_RADIO_EVENT_READY))
    {
        nrf_radio_event_clear(NRF_RADIO_EVENT_READY);

        switch (m_state)
        {
        case RADIO_STATE_WAITING_RX_FRAME:
            assert_tifs_shorts_disabled();

            assert(nrf_radio_state_get() == NRF_RADIO_STATE_RXIDLE);

            if ((mp_current_rx_buffer != NULL) && (mp_current_rx_buffer->free))
            {
                if (mutex_lock())
                {
                    shorts_tifs_initial_enable();
                    rx_start();

                    mutex_unlock();
                }
            }

            break;

        case RADIO_STATE_TX_ACK:
            assert_tifs_shorts_enabled();
            shorts_tifs_ack_disable();
            break;

        case RADIO_STATE_CCA:
            assert_tifs_shorts_disabled();

            if (nrf_radio_state_get() != NRF_RADIO_STATE_RXIDLE)
            {
                assert(false);
            }

            state_set(RADIO_STATE_TX_FRAME);

            shorts_tx_frame_enable();
            nrf_radio_task_trigger(NRF_RADIO_TASK_CCASTART);
            break;

        case RADIO_STATE_TX_FRAME:
            assert_tifs_shorts_disabled();
            break;

        case RADIO_STATE_RX_ACK:
            assert(nrf_radio_state_get() == NRF_RADIO_STATE_RXIDLE);
            assert_tifs_shorts_disabled();
            rx_start(); // Reuse buffer used by interrupted rx procedure.
            break;

        case RADIO_STATE_ED:
            assert_tifs_shorts_disabled();
            assert(nrf_radio_state_get() == NRF_RADIO_STATE_RXIDLE);
            nrf_radio_task_trigger(NRF_RADIO_TASK_EDSTART);
            break;

        default:
            assert(false);
        }
    }

#if !SHORT_CCAIDLE_TXEN

    if (nrf_radio_event_check(NRF_RADIO_EVENT_CCAIDLE))
    {
        assert (m_state == RADIO_STATE_TX_FRAME);

        disableTransceiver();

        nrf_radio_event_clear(NRF_RADIO_EVENT_CCAIDLE);
    }

#endif

    if (nrf_radio_event_check(NRF_RADIO_EVENT_CCABUSY))
    {
        assert(nrf_radio_state_get() == NRF_RADIO_STATE_RXIDLE);
        assert(m_state == RADIO_STATE_TX_FRAME);
        assert_tifs_shorts_disabled();
        shorts_tx_frame_disable();

        nrf_drv_radio802154_busy_channel();

        state_set(RADIO_STATE_WAITING_RX_FRAME);
        rx_enable();

        nrf_radio_event_clear(NRF_RADIO_EVENT_CCABUSY);
    }

    if (nrf_radio_event_check(NRF_RADIO_EVENT_EDEND))
    {
        nrf_radio_event_clear(NRF_RADIO_EVENT_EDEND);

        assert(nrf_radio_state_get() == NRF_RADIO_STATE_RXIDLE);
        assert(m_state == RADIO_STATE_ED);
        assert_tifs_shorts_disabled();

        nrf_drv_radio802154_energy_detected(last_ed_result_get());

        state_set(RADIO_STATE_WAITING_RX_FRAME);
        rx_enable();
    }
}

#ifdef RADIO_IRQ_CTRL
void RADIO_IRQHandler(void)
{
  nrf_drv_radio802154_irq_handler();
}
#endif

void __attribute__((weak)) nrf_drv_radio802154_received(uint8_t * p_data, int8_t power, int8_t lqi)
{
    (void) p_data;
    (void) power;
    (void) lqi;
}

void __attribute__((weak)) nrf_drv_radio802154_transmitted(bool pending_bit)
{
    (void) pending_bit;
}

void __attribute__((weak)) nrf_drv_radio802154_busy_channel(void)
{

}

void __attribute__((weak)) nrf_drv_radio802154_energy_detected(int8_t result)
{
    (void) result;
}
