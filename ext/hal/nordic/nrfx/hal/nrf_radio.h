/* Copyright (c) 2016-2017 Nordic Semiconductor ASA
 *
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
 * @brief RADIO HAL API.
 */

#ifndef NRF_RADIO_H__
#define NRF_RADIO_H__
/**
 * @defgroup nrf_radio_hal RADIO HAL
 * @{
 * @ingroup nrf_radio
 * @brief Hardware access layer for managing the radio (RADIO).
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "nrf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NRF_RADIO_TASK_SET    (1UL)
#define NRF_RADIO_EVENT_CLEAR (0UL)
/**
 * @enum nrf_radio_task_t
 * @brief RADIO tasks.
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    NRF_RADIO_TASK_TXEN      = offsetof(NRF_RADIO_Type, TASKS_TXEN),      /**< Enable radio transmitter. */
    NRF_RADIO_TASK_RXEN      = offsetof(NRF_RADIO_Type, TASKS_RXEN),      /**< Enable radio receiver. */
    NRF_RADIO_TASK_START     = offsetof(NRF_RADIO_Type, TASKS_START),     /**< Start radio transmission or reception. */
    NRF_RADIO_TASK_STOP      = offsetof(NRF_RADIO_Type, TASKS_STOP),      /**< Stop radio transmission or reception. */
    NRF_RADIO_TASK_DISABLE   = offsetof(NRF_RADIO_Type, TASKS_DISABLE),   /**< Disable radio transmitter and receiver. */
    NRF_RADIO_TASK_CCASTART  = offsetof(NRF_RADIO_Type, TASKS_CCASTART),  /**< Start Clear Channel Assessment procedure. */
    NRF_RADIO_TASK_CCASTOP   = offsetof(NRF_RADIO_Type, TASKS_CCASTOP),   /**< Stop Clear Channel Assessment procedure. */
    NRF_RADIO_TASK_EDSTART   = offsetof(NRF_RADIO_Type, TASKS_EDSTART),   /**< Start Energy Detection procedure. */
    NRF_RADIO_TASK_RSSISTART = offsetof(NRF_RADIO_Type, TASKS_RSSISTART), /**< Start the RSSI and take one single sample of received signal strength. */
} nrf_radio_task_t;                                                       /*lint -restore */

/**
 * @enum nrf_radio_event_t
 * @brief RADIO events.
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    NRF_RADIO_EVENT_READY      = offsetof(NRF_RADIO_Type, EVENTS_READY),      /**< Radio has ramped up and is ready to be started. */
    NRF_RADIO_EVENT_ADDRESS    = offsetof(NRF_RADIO_Type, EVENTS_ADDRESS),    /**< Address sent or received. */
    NRF_RADIO_EVENT_END        = offsetof(NRF_RADIO_Type, EVENTS_END),        /**< Packet transmitted or received. */
    NRF_RADIO_EVENT_DISABLED   = offsetof(NRF_RADIO_Type, EVENTS_DISABLED),   /**< Radio has been disabled. */
    NRF_RADIO_EVENT_CRCOK      = offsetof(NRF_RADIO_Type, EVENTS_CRCOK),      /**< Packet received with correct CRC. */
    NRF_RADIO_EVENT_CRCERROR   = offsetof(NRF_RADIO_Type, EVENTS_CRCERROR),   /**< Packet received with incorrect CRC. */
    NRF_RADIO_EVENT_CCAIDLE    = offsetof(NRF_RADIO_Type, EVENTS_CCAIDLE),    /**< Wireless medium is idle. */
    NRF_RADIO_EVENT_CCABUSY    = offsetof(NRF_RADIO_Type, EVENTS_CCABUSY),    /**< Wireless medium is busy. */
    NRF_RADIO_EVENT_RSSIEND    = offsetof(NRF_RADIO_Type, EVENTS_RSSIEND),    /**< Sampling of receive signal strength complete. */
    NRF_RADIO_EVENT_MHRMATCH   = offsetof(NRF_RADIO_Type, EVENTS_MHRMATCH),   /**< MAC Header match found. */
    NRF_RADIO_EVENT_FRAMESTART = offsetof(NRF_RADIO_Type, EVENTS_FRAMESTART), /**< IEEE 802.15.4 length field received. */
    NRF_RADIO_EVENT_EDEND      = offsetof(NRF_RADIO_Type, EVENTS_EDEND),      /**< Energy Detection procedure ended. */
    NRF_RADIO_EVENT_BCMATCH    = offsetof(NRF_RADIO_Type, EVENTS_BCMATCH),    /**< Bit counter reached bit count value. */
} nrf_radio_event_t;                                                          /*lint -restore */

/**
 * @enum nrf_radio_int_mask_t
 * @brief RADIO interrupts.
 */
typedef enum
{
    NRF_RADIO_INT_READY_MASK      = RADIO_INTENSET_READY_Msk,      /**< Mask for enabling or disabling an interrupt on READY event.  */
    NRF_RADIO_INT_ADDRESS_MASK    = RADIO_INTENSET_ADDRESS_Msk,    /**< Mask for enabling or disabling an interrupt on ADDRESS event. */
    NRF_RADIO_INT_END_MASK        = RADIO_INTENSET_END_Msk,        /**< Mask for enabling or disabling an interrupt on END event. */
    NRF_RADIO_INT_DISABLED_MASK   = RADIO_INTENSET_DISABLED_Msk,   /**< Mask for enabling or disabling an interrupt on DISABLED event. */
    NRF_RADIO_INT_CRCOK_MASK      = RADIO_INTENSET_CRCOK_Msk,      /**< Mask for enabling or disabling an interrupt on CRCOK event. */
    NRF_RADIO_INT_CRCERROR_MASK   = RADIO_INTENSET_CRCERROR_Msk,   /**< Mask for enabling or disabling an interrupt on CRCERROR event. */
    NRF_RADIO_INT_CCAIDLE_MASK    = RADIO_INTENSET_CCAIDLE_Msk,    /**< Mask for enabling or disabling an interrupt on CCAIDLE event. */
    NRF_RADIO_INT_CCABUSY_MASK    = RADIO_INTENSET_CCABUSY_Msk,    /**< Mask for enabling or disabling an interrupt on CCABUSY event. */
    NRF_RADIO_INT_RSSIEND_MASK    = RADIO_INTENSET_RSSIEND_Msk,    /**< Mask for enabling or disabling an interrupt on RSSIEND event. */
    NRF_RADIO_INT_FRAMESTART_MASK = RADIO_INTENSET_FRAMESTART_Msk, /**< Mask for enabling or disabling an interrupt on FRAMESTART event. */
    NRF_RADIO_INT_EDEND_MASK      = RADIO_INTENSET_EDEND_Msk,      /**< Mask for enabling or disabling an interrupt on EDEND event. */
    NRF_RADIO_INT_BCMATCH_MASK    = RADIO_INTENSET_BCMATCH_Msk,    /**< Mask for enabling or disabling an interrupt on BCMATCH event. */
} nrf_radio_int_mask_t;

/**
 * @enum nrf_radio_short_mask_t
 * @brief Types of RADIO shortcuts.
 */
typedef enum
{
    NRF_RADIO_SHORT_READY_START_MASK        = RADIO_SHORTS_READY_START_Msk,        /**< Mask for setting shortcut between EVENT_READY and TASK_START. */
    NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK  = RADIO_SHORTS_ADDRESS_RSSISTART_Msk,  /**< Mask for setting shortcut between EVENT_ADDRESS and TASK_RSSISTART. */
    NRF_RADIO_SHORT_END_DISABLE_MASK        = RADIO_SHORTS_END_DISABLE_Msk,        /**< Mask for setting shortcut between EVENT_END and TASK_DISABLE. */
    NRF_RADIO_SHORT_END_START_MASK          = RADIO_SHORTS_END_START_Msk,          /**< Mask for setting shortcut between EVENT_END and TASK_START. */
    NRF_RADIO_SHORT_CCAIDLE_TXEN_MASK       = RADIO_SHORTS_CCAIDLE_TXEN_Msk,       /**< Mask for setting shortcut between EVENT_CCAIDLE and TASK_TXEN. */
    NRF_RADIO_SHORT_DISABLED_TXEN_MASK      = RADIO_SHORTS_DISABLED_TXEN_Msk,      /**< Mask for setting shortcut between EVENT_DISABLED and TASK_TXEN. */
    NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK = RADIO_SHORTS_FRAMESTART_BCSTART_Msk, /**< Mask for setting shortcut between EVENT_FRAMESTART and TASK_BCSTART. */
} nrf_radio_short_mask_t;

/**
 * @enum nrf_radio_cca_mode_t
 * @brief Types of RADIO Clear Channel Assessment modes.
 */
typedef enum
{
    NRF_RADIO_CCA_MODE_ED             = RADIO_CCACTRL_CCAMODE_EdMode,           /**< Energy Above Threshold. Will report busy whenever energy is detected above set threshold. */
    NRF_RADIO_CCA_MODE_CARRIER        = RADIO_CCACTRL_CCAMODE_CarrierMode,      /**< Carrier Seen. Will report busy whenever compliant IEEE 802.15.4 signal is seen. */
    NRF_RADIO_CCA_MODE_CARRIER_AND_ED = RADIO_CCACTRL_CCAMODE_CarrierAndEdMode, /**< Energy Above Threshold AND Carrier Seen. */
    NRF_RADIO_CCA_MODE_CARRIER_OR_ED  = RADIO_CCACTRL_CCAMODE_CarrierOrEdMode,  /**< Energy Above Threshold OR Carrier Seen. */
} nrf_radio_cca_mode_t;

/**
 * @enum nrf_radio_state_t
 * @brief Types of RADIO States.
 */
typedef enum
{
    NRF_RADIO_STATE_DISABLED   = RADIO_STATE_STATE_Disabled,  /**< No operations are going on inside the radio and the power consumption is at a minimum. */
    NRF_RADIO_STATE_RX_RU      = RADIO_STATE_STATE_RxRu,      /**< The radio is ramping up and preparing for reception. */
    NRF_RADIO_STATE_RX_IDLE    = RADIO_STATE_STATE_RxIdle,    /**< The radio is ready for reception to start. */
    NRF_RADIO_STATE_RX         = RADIO_STATE_STATE_Rx,        /**< Reception has been started. */
    NRF_RADIO_STATE_RX_DISABLE = RADIO_STATE_STATE_RxDisable, /**< The radio is disabling the receiver. */
    NRF_RADIO_STATE_TX_RU      = RADIO_STATE_STATE_TxRu,      /**< The radio is ramping up and preparing for transmission. */
    NRF_RADIO_STATE_TX_IDLE    = RADIO_STATE_STATE_TxIdle,    /**< The radio is ready for transmission to start. */
    NRF_RADIO_STATE_TX         = RADIO_STATE_STATE_Tx,        /**< The radio is transmitting a packet. */
    NRF_RADIO_STATE_TX_DISABLE = RADIO_STATE_STATE_TxDisable, /**< The radio is disabling the transmitter. */
} nrf_radio_state_t;

/**
 * @enum nrf_radio_crc_status_t
 * @brief Types of CRC status.
 */
typedef enum
{
    NRF_RADIO_CRC_STATUS_ERROR = RADIO_CRCSTATUS_CRCSTATUS_CRCError, /**< Packet received with CRC error. */
    NRF_RADIO_CRC_STATUS_OK    = RADIO_CRCSTATUS_CRCSTATUS_CRCOk,    /**< Packet received with CRC ok. */
} nrf_radio_crc_status_t;

/**
 * @enum nrf_radio_mode_t
 * @brief Types of RADIO modes (data rate and modulation).
 */
typedef enum
{
    NRF_RADIO_MODE_NRF_1MBIT          = RADIO_MODE_MODE_Nrf_1Mbit,          /**< 1Mbit/s Nordic proprietary radio mode. */
    NRF_RADIO_MODE_NRF_2MBIT          = RADIO_MODE_MODE_Nrf_2Mbit,          /**< 2Mbit/s Nordic proprietary radio mode. */
    NRF_RADIO_MODE_BLE_1MBIT          = RADIO_MODE_MODE_Ble_1Mbit,          /**< 1 Mbit/s Bluetooth Low Energy. */
    NRF_RADIO_MODE_BLE_2MBIT          = RADIO_MODE_MODE_Ble_2Mbit,          /**< 2 Mbit/s Bluetooth Low Energy. */
    NRF_RADIO_MODE_IEEE802154_250KBIT = RADIO_MODE_MODE_Ieee802154_250Kbit, /**< IEEE 802.15.4-2006 250 kbit/s. */
} nrf_radio_mode_t;

/**
 * @enum nrf_radio_preamble_length_t
 * @brief Types of preamble length.
 */
typedef enum
{
    NRF_RADIO_PREAMBLE_LENGTH_8BIT       = RADIO_PCNF0_PLEN_8bit,      /**< 8-bit preamble. */
    NRF_RADIO_PREAMBLE_LENGTH_16BIT      = RADIO_PCNF0_PLEN_16bit,     /**< 16-bit preamble. */
    NRF_RADIO_PREAMBLE_LENGTH_32BIT_ZERO = RADIO_PCNF0_PLEN_32bitZero, /**< 32-bit zero preamble used for IEEE 802.15.4. */
    NRF_RADIO_PREAMBLE_LENGTH_LONG_RANGE = RADIO_PCNF0_PLEN_LongRange, /**< Preamble - used for BTLE Long Range. */
} nrf_radio_preamble_length_t;

/**
 * @enum nrf_radio_crc_includes_addr_t
 * @brief Types of CRC calculatons regarding address.
 */
typedef enum
{
    NRF_RADIO_CRC_INCLUDES_ADDR_INCLUDE    = RADIO_CRCCNF_SKIPADDR_Include,    /**< CRC calculation includes address field. */
    NRF_RADIO_CRC_INCLUDES_ADDR_SKIP       = RADIO_CRCCNF_SKIPADDR_Skip,       /**< CRC calculation does not include address field. */
    NRF_RADIO_CRC_INCLUDES_ADDR_IEEE802154 = RADIO_CRCCNF_SKIPADDR_Ieee802154, /**< CRC calculation as per 802.15.4 standard. */
} nrf_radio_crc_includes_addr_t;

/**
 * @brief Function for enabling interrupts.
 *
 * @param[in]  radio_int_mask              Mask of interrupts.
 */
__STATIC_INLINE void nrf_radio_int_enable(uint32_t radio_int_mask)
{
    NRF_RADIO->INTENSET = radio_int_mask;
}

/**
 * @brief Function for disabling interrupts.
 *
 * @param[in]  radio_int_mask              Mask of interrupts.
 */
__STATIC_INLINE void nrf_radio_int_disable(uint32_t radio_int_mask)
{
    NRF_RADIO->INTENCLR = radio_int_mask;
}

/**
 * @brief Function for getting the state of a specific interrupt.
 *
 * @param[in]  radio_int_mask              Interrupt.
 *
 * @retval     true                   If the interrupt is not enabled.
 * @retval     false                  If the interrupt is enabled.
 */
__STATIC_INLINE bool nrf_radio_int_get(nrf_radio_int_mask_t radio_int_mask)
{
    return (bool)(NRF_RADIO->INTENCLR & radio_int_mask);
}

/**
 * @brief Function for getting the address of a specific task.
 *
 * This function can be used by the PPI module.
 *
 * @param[in]  radio_task              Task.
 */
__STATIC_INLINE uint32_t *nrf_radio_task_address_get(nrf_radio_task_t radio_task)
{
    return (uint32_t *)((uint8_t *)NRF_RADIO + radio_task);
}

/**
 * @brief Function for setting a specific task.
 *
 * @param[in]  radio_task              Task.
 */
__STATIC_INLINE void nrf_radio_task_trigger(nrf_radio_task_t radio_task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_RADIO + radio_task)) = NRF_RADIO_TASK_SET;
}

/**
 * @brief Function for getting address of a specific event.
 *
 * This function can be used by the PPI module.
 *
 * @param[in]  radio_event              Event.
 */
__STATIC_INLINE uint32_t *nrf_radio_event_address_get(nrf_radio_event_t radio_event)
{
    return (uint32_t *)((uint8_t *)NRF_RADIO + radio_event);
}

/**
 * @brief Function for clearing a specific event.
 *
 * @param[in]  radio_event              Event.
 */
__STATIC_INLINE void nrf_radio_event_clear(nrf_radio_event_t radio_event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_RADIO + radio_event)) = NRF_RADIO_EVENT_CLEAR;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_RADIO + radio_event));
    (void)dummy;
#endif
}

/**
 * @brief Function for getting the state of a specific event.
 *
 * @param[in]  radio_event              Event.
 *
 * @retval     true               If the event is not set.
 * @retval     false              If the event is set.
 */
__STATIC_INLINE bool nrf_radio_event_get(nrf_radio_event_t radio_event)
{
    return (bool) * ((volatile uint32_t *)((uint8_t *)NRF_RADIO + radio_event));
}

/**
 * @brief Function for setting shortcuts.
 *
 * @param[in]  radio_short_mask              Mask of shortcuts.
 *
 */
__STATIC_INLINE void nrf_radio_shorts_enable(uint32_t radio_short_mask)
{
    NRF_RADIO->SHORTS |= radio_short_mask;
}

/**
 * @brief Function for clearing shortcuts.
 *
 * @param[in]  radio_short_mask              Mask of shortcuts.
 *
 */
__STATIC_INLINE void nrf_radio_shorts_disable(uint32_t radio_short_mask)
{
    NRF_RADIO->SHORTS &= ~radio_short_mask;
}

/**
 * @brief Function for getting present state of the radio module.
 */
__STATIC_INLINE nrf_radio_state_t nrf_radio_state_get(void)
{
    return (nrf_radio_state_t) NRF_RADIO->STATE;
}

/**
 * @brief Function for setting Packet Pointer to given location in memory.
 *
 * @param[in]  p_radio_packet_ptr           Pointer to tx or rx packet buffer.
 */
__STATIC_INLINE void nrf_radio_packet_ptr_set(const void *p_radio_packet_ptr)
{
    NRF_RADIO->PACKETPTR = (uint32_t) p_radio_packet_ptr;
}

/**
 * @brief Function for getting CRC status of last received packet.
 */
__STATIC_INLINE nrf_radio_crc_status_t nrf_radio_crc_status_get(void)
{
    return (nrf_radio_crc_status_t) NRF_RADIO->CRCSTATUS;
}

/**
 * @brief Function for setting Clear Channel Assessment mode.
 *
 * @param[in]  radio_cca_mode                Mode of CCA
 */
__STATIC_INLINE void nrf_radio_cca_mode_set(nrf_radio_cca_mode_t radio_cca_mode)
{
    NRF_RADIO->CCACTRL &= (~RADIO_CCACTRL_CCAMODE_Msk);
    NRF_RADIO->CCACTRL |= ((uint32_t) radio_cca_mode << RADIO_CCACTRL_CCAMODE_Pos);
}

/**
 * @brief Function for setting CCA Energy Busy Threshold.
 *
 * @param[in]  radio_cca_ed_threshold        Energy Detection threshold value.
 */
__STATIC_INLINE void nrf_radio_cca_ed_threshold_set(uint8_t radio_cca_ed_threshold)
{
    NRF_RADIO->CCACTRL &= (~RADIO_CCACTRL_CCAEDTHRES_Msk);
    NRF_RADIO->CCACTRL |= ((uint32_t) radio_cca_ed_threshold << RADIO_CCACTRL_CCAEDTHRES_Pos);
}

/**
 * @brief Function for setting CCA Correlator Busy Threshold.
 *
 * @param[in]  radio_cca_corr_threshold      Correlator Busy Threshold.
 */
__STATIC_INLINE void nrf_radio_cca_corr_threshold_set(uint8_t radio_cca_corr_threshold_set)
{
    NRF_RADIO->CCACTRL &= (~RADIO_CCACTRL_CCACORRTHRES_Msk);
    NRF_RADIO->CCACTRL |= ((uint32_t) radio_cca_corr_threshold_set << RADIO_CCACTRL_CCACORRTHRES_Pos);
}

/**
 * @brief Function for setting limit of occurances above Correlator Threshold.
 *
 * When not equal to zero the correlator based signal detect is enabled.
 *
 * @param[in]  radio_cca_corr_cnt           Limit of occurances above Correlator Threshold
 */
__STATIC_INLINE void nrf_radio_cca_corr_counter_set(uint8_t radio_cca_corr_counter_set)
{
    NRF_RADIO->CCACTRL &= (~RADIO_CCACTRL_CCACORRCNT_Msk);
    NRF_RADIO->CCACTRL |= ((uint32_t) radio_cca_corr_counter_set << RADIO_CCACTRL_CCACORRCNT_Pos);
}

/**
 * @brief Function for setting Radio data rate and modulation settings.
 *
 * @param[in]  radio_mode                   Mode of radio data rate and modulation.
 */
__STATIC_INLINE void nrf_radio_mode_set(nrf_radio_mode_t radio_mode)
{
    NRF_RADIO->MODE = ((uint32_t) radio_mode << RADIO_MODE_MODE_Pos);
}

/**
 * @brief Function for setting Length of LENGTH field in number of bits.
 *
 * @param[in]  radio_length_length          Length of LENGTH field in number of bits.
 */
__STATIC_INLINE void nrf_radio_config_length_field_length_set(uint8_t radio_length_length)
{
    NRF_RADIO->PCNF0 &= (~RADIO_PCNF0_LFLEN_Msk);
    NRF_RADIO->PCNF0 |= ((uint32_t) radio_length_length << RADIO_PCNF0_LFLEN_Pos);
}

/**
 * @brief Function for setting length of preamble on air.
 *
 * @param[in]  radio_preamble_length        Length of preamble on air.
 */
__STATIC_INLINE void nrf_radio_config_preamble_length_set(
    nrf_radio_preamble_length_t radio_preamble_length)
{
    NRF_RADIO->PCNF0 &= (~RADIO_PCNF0_PLEN_Msk);
    NRF_RADIO->PCNF0 |= ((uint32_t) radio_preamble_length << RADIO_PCNF0_PLEN_Pos);
}

/**
 * @brief Function for setting if LENGTH field contains CRC.
 *
 * @param[in]  radio_length_contains_crc    True if LENGTH field should contain CRC.
 */
__STATIC_INLINE void nrf_radio_config_crc_included_set(bool radio_length_contains_crc)
{
    NRF_RADIO->PCNF0 &= (~RADIO_PCNF0_CRCINC_Msk);
    NRF_RADIO->PCNF0 |= ((uint32_t) radio_length_contains_crc << RADIO_PCNF0_CRCINC_Pos);
}

/**
 * @brief Function for setting maximum length of packet payload.
 *
 * @param[in]  radio_max_packet_length      Maximum length of packet payload.
 */
__STATIC_INLINE void nrf_radio_config_max_length_set(uint8_t radio_max_packet_length)
{
    NRF_RADIO->PCNF1 &= (~RADIO_PCNF1_MAXLEN_Msk);
    NRF_RADIO->PCNF1 |= ((uint32_t) radio_max_packet_length << RADIO_PCNF1_MAXLEN_Pos);
}

/**
 * @brief Function for setting CRC length.
 *
 * @param[in]  radio_crc_length             CRC length in number of bytes [0-3].
 */
__STATIC_INLINE void nrf_radio_crc_length_set(uint8_t radio_crc_length)
{
    NRF_RADIO->CRCCNF &= (~RADIO_CRCCNF_LEN_Msk);
    NRF_RADIO->CRCCNF |= ((uint32_t) radio_crc_length << RADIO_CRCCNF_LEN_Pos);
}

/**
 * @brief Function for setting if address filed should be included or excluded from CRC calculation.
 *
 * @param[in]  radio_crc_skip_address       Include or exclude packet address field out of CRC.
 */
__STATIC_INLINE void nrf_radio_crc_includes_address_set(
    nrf_radio_crc_includes_addr_t radio_crc_skip_address)
{
    NRF_RADIO->CRCCNF &= (~RADIO_CRCCNF_SKIPADDR_Msk);
    NRF_RADIO->CRCCNF |= ((uint32_t) radio_crc_skip_address << RADIO_CRCCNF_SKIPADDR_Pos);
}

/**
 * @brief Function for setting CRC polynominal.
 *
 * @param[in]  radio_crc_polynominal        CRC polynominal to set.
 */
__STATIC_INLINE void nrf_radio_crc_polynominal_set(uint32_t radio_crc_polynominal)
{
    NRF_RADIO->CRCPOLY = (radio_crc_polynominal << RADIO_CRCPOLY_CRCPOLY_Pos);
}

/**
 * @brief Function for getting RSSI sample result.
 *
 * @note The value is a positive value while the actual received signal is a negative value.
 *       Actual received signal strength is therefore as follows:
 *       received signal strength = - returned_value dBm .
 */
__STATIC_INLINE uint8_t nrf_radio_rssi_sample_get(void)
{
    return (uint8_t)(((NRF_RADIO->RSSISAMPLE) & RADIO_RSSISAMPLE_RSSISAMPLE_Msk) >>
                     RADIO_RSSISAMPLE_RSSISAMPLE_Pos);
}

/**
 * @brief Function for setting MAC Header Match Unit search pattern configuration.
 *
 * @param[in]  radio_mhmu_search_pattern    Search Pattern Configuration.
 */
__STATIC_INLINE void nrf_radio_mhmu_search_pattern_set(uint32_t radio_mhmu_search_pattern)
{
    NRF_RADIO->MHRMATCHCONF = radio_mhmu_search_pattern;
}

/**
 * @brief Function for setting MAC Header Match Unit pattern mask configuration.
 *
 * @param[in]  radio_mhmu_pattern_mask      Pattern mask.
 */
__STATIC_INLINE void nrf_radio_mhmu_pattern_mask_set(uint32_t radio_mhmu_pattern_mask)
{
    NRF_RADIO->MHRMATCHMAS = radio_mhmu_pattern_mask;
}

/**
 * @brief Function for setting radio frequency.
 *
 * @param[in]  radio_frequency              Frequency above 2400 MHz [MHz]
 */
__STATIC_INLINE void nrf_radio_frequency_set(uint32_t radio_frequency)
{
    NRF_RADIO->FREQUENCY = radio_frequency;
}

/**
 * @brief Function for getting radio frequency.
 *
 * @returns Frequency above 2400 MHz [MHz]
 */
__STATIC_INLINE uint32_t nrf_radio_frequency_get(void)
{
    return NRF_RADIO->FREQUENCY;
}

/**
 * @brief Function for setting radio transmit power.
 *
 * @param[in]  radio_tx_power               Transmit power of the radio.
 */
__STATIC_INLINE void nrf_radio_tx_power_set(int8_t radio_tx_power)
{
    NRF_RADIO->TXPOWER = (uint8_t) radio_tx_power;
}

/**
 * @brief Function for setting Inter Frame Spacing
 *
 * @param[in]  radio_ifs                    Inter frame spacing [us].
 */
__STATIC_INLINE void nrf_radio_ifs_set(uint32_t radio_ifs)
{
    NRF_RADIO->TIFS = radio_ifs;
}

/**
 * @brief Function for setting Bit counter compare.
 *
 * @param[in]  radio_bcc                    Bit counter compare [bits].
 */
__STATIC_INLINE void nrf_radio_bcc_set(uint32_t radio_bcc)
{
    NRF_RADIO->BCC = radio_bcc;
}

/**
 * @brief Function for getting Bit counter compare.
 */
__STATIC_INLINE uint32_t nrf_radio_bcc_get(void)
{
    return NRF_RADIO->BCC;
}

/**
 * @brief Function for getting Energy Detection level.
 */
__STATIC_INLINE uint8_t nrf_radio_ed_sample_get(void)
{
    return (uint8_t) NRF_RADIO->EDSAMPLE;
}

/**
 * @brief Function for setting number of iterations to perform ED scan.
 *
 * @param[in]  radio_ed_loop_count          Number of iterations during ED procedure.
 */
__STATIC_INLINE void nrf_radio_ed_loop_count_set(uint32_t radio_ed_loop_count)
{
    NRF_RADIO->EDCNT = (radio_ed_loop_count & 0x001FFFFF);
}

/**
 * @brief Function for setting power mode of the radio peripheral.
 *
 * @param[in]  radio_power                  If radio should powered on.
 */
__STATIC_INLINE void nrf_radio_power_set(bool radio_power)
{
    NRF_RADIO->POWER = (uint32_t) radio_power;
}


/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif /* NRF_RADIO_H__ */
