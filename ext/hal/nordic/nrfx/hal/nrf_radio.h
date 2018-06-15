/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
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

#ifndef NRF_RADIO_H__
#define NRF_RADIO_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_radio_hal RADIO HAL
 * @{
 * @ingroup nrf_radio
 * @brief   Hardware access layer for managing the RADIO peripheral.
 */

/**
 * @brief RADIO tasks.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_RADIO_TASK_TXEN      = offsetof(NRF_RADIO_Type, TASKS_TXEN),      /**< Enable RADIO in TX mode. */
    NRF_RADIO_TASK_RXEN      = offsetof(NRF_RADIO_Type, TASKS_RXEN),      /**< Enable RADIO in RX mode. */
    NRF_RADIO_TASK_START     = offsetof(NRF_RADIO_Type, TASKS_START),     /**< Start RADIO. */
    NRF_RADIO_TASK_STOP      = offsetof(NRF_RADIO_Type, TASKS_STOP),      /**< Stop RADIO. */
    NRF_RADIO_TASK_DISABLE   = offsetof(NRF_RADIO_Type, TASKS_DISABLE),   /**< Disable RADIO. */
    NRF_RADIO_TASK_RSSISTART = offsetof(NRF_RADIO_Type, TASKS_RSSISTART), /**< Start the RSSI and take one single sample of the receive signal strength. */
    NRF_RADIO_TASK_RSSISTOP  = offsetof(NRF_RADIO_Type, TASKS_RSSISTOP),  /**< Stop the RSSI measurement. */
    NRF_RADIO_TASK_BCSTART   = offsetof(NRF_RADIO_Type, TASKS_BCSTART),   /**< Start the bit counter. */
    NRF_RADIO_TASK_BCSTOP    = offsetof(NRF_RADIO_Type, TASKS_BCSTOP),    /**< Stop the bit counter. */
#if defined(RADIO_TASKS_EDSTART_TASKS_EDSTART_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TASK_EDSTART   = offsetof(NRF_RADIO_Type, TASKS_EDSTART),   /**< Start the Energy Detect measurement used in IEEE 802.15.4 mode. */
#endif
#if defined(RADIO_TASKS_EDSTOP_TASKS_EDSTOP_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TASK_EDSTOP    = offsetof(NRF_RADIO_Type, TASKS_EDSTOP),    /**< Stop the Energy Detect measurement. */
#endif
#if defined(RADIO_TASKS_CCASTART_TASKS_CCASTART_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TASK_CCASTART  = offsetof(NRF_RADIO_Type, TASKS_CCASTART),  /**< Start the Clear Channel Assessment used in IEEE 802.15.4 mode. */
#endif
#if defined(RADIO_TASKS_CCASTOP_TASKS_CCASTOP_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TASK_CCASTOP   = offsetof(NRF_RADIO_Type, TASKS_CCASTOP),   /**< Stop the Clear Channel Assessment. */
#endif
    /*lint -restore*/
} nrf_radio_task_t;

/**
 * @brief RADIO events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_RADIO_EVENT_READY      = offsetof(NRF_RADIO_Type, EVENTS_READY),      /**< Radio has ramped up and is ready to be started. */
    NRF_RADIO_EVENT_ADDRESS    = offsetof(NRF_RADIO_Type, EVENTS_ADDRESS),    /**< Address sent or received. */
    NRF_RADIO_EVENT_PAYLOAD    = offsetof(NRF_RADIO_Type, EVENTS_PAYLOAD),    /**< Packet payload sent or received. */
    NRF_RADIO_EVENT_END        = offsetof(NRF_RADIO_Type, EVENTS_END),        /**< Packet transmitted or received. */
    NRF_RADIO_EVENT_DISABLED   = offsetof(NRF_RADIO_Type, EVENTS_DISABLED),   /**< RADIO has been disabled. */
    NRF_RADIO_EVENT_DEVMATCH   = offsetof(NRF_RADIO_Type, EVENTS_DEVMATCH),   /**< A device address match occurred on the last received packet. */
    NRF_RADIO_EVENT_DEVMISS    = offsetof(NRF_RADIO_Type, EVENTS_DEVMISS),    /**< No device address match occurred on the last received packet. */
    NRF_RADIO_EVENT_RSSIEND    = offsetof(NRF_RADIO_Type, EVENTS_RSSIEND),    /**< Sampling of receive signal strength complete. */
    NRF_RADIO_EVENT_BCMATCH    = offsetof(NRF_RADIO_Type, EVENTS_BCMATCH),    /**< Bit counter reached bit count value. */
#if defined(RADIO_INTENSET_CRCOK_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_CRCOK      = offsetof(NRF_RADIO_Type, EVENTS_CRCOK),      /**< Packet received with correct CRC. */
#endif
#if defined(RADIO_INTENSET_CRCERROR_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_CRCERROR   = offsetof(NRF_RADIO_Type, EVENTS_CRCERROR),   /**< Packet received with incorrect CRC. */
#endif
#if defined(RADIO_INTENSET_FRAMESTART_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_FRAMESTART = offsetof(NRF_RADIO_Type, EVENTS_FRAMESTART), /**< IEEE 802.15.4 length field received. */
#endif
#if defined(RADIO_INTENSET_EDEND_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_EDEND      = offsetof(NRF_RADIO_Type, EVENTS_EDEND),      /**< Energy Detection procedure ended. */
#endif
#if defined(RADIO_INTENSET_EDSTOPPED_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_EDSTOPPED  = offsetof(NRF_RADIO_Type, EVENTS_EDSTOPPED),  /**< The sampling of Energy Detection has stopped. */
#endif
#if defined(RADIO_INTENSET_CCAIDLE_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_CCAIDLE    = offsetof(NRF_RADIO_Type, EVENTS_CCAIDLE),    /**< Wireless medium in idle - clear to send. */
#endif
#if defined(RADIO_INTENSET_CCABUSY_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_CCABUSY    = offsetof(NRF_RADIO_Type, EVENTS_CCABUSY),    /**< Wireless medium busy - do not send. */
#endif
#if defined(RADIO_INTENSET_CCASTOPPED_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_CCASTOPPED = offsetof(NRF_RADIO_Type, EVENTS_CCASTOPPED), /**< The CCA has stopped. */
#endif
#if defined(RADIO_INTENSET_RATEBOOST_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_RATEBOOST  = offsetof(NRF_RADIO_Type, EVENTS_RATEBOOST),  /**< Ble_LR CI field received, receive mode is changed from Ble_LR125Kbit to Ble_LR500Kbit. */
#endif
#if defined(RADIO_INTENSET_TXREADY_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_TXREADY    = offsetof(NRF_RADIO_Type, EVENTS_TXREADY),    /**< RADIO has ramped up and is ready to be started TX path. */
#endif
#if defined(RADIO_INTENSET_RXREADY_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_RXREADY    = offsetof(NRF_RADIO_Type, EVENTS_RXREADY),    /**< RADIO has ramped up and is ready to be started RX path. */
#endif
#if defined(RADIO_INTENSET_MHRMATCH_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_MHRMATCH   = offsetof(NRF_RADIO_Type, EVENTS_MHRMATCH),   /**< MAC Header match found. */
#endif
#if defined(RADIO_INTENSET_PHYEND_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_EVENT_PHYEND     = offsetof(NRF_RADIO_Type, EVENTS_PHYEND),     /**< Generated in Ble_LR125Kbit, Ble_LR500Kbit
                                                                                   and BleIeee802154_250Kbit modes when last
                                                                                   bit is sent on the air. */
#endif
    /*lint -restore*/
} nrf_radio_event_t;

/**
 * @brief RADIO interrupts.
 */
typedef enum
{
    NRF_RADIO_INT_READY_MASK      = RADIO_INTENSET_READY_Msk,      /**< Interrupt on READY event.  */
    NRF_RADIO_INT_ADDRESS_MASK    = RADIO_INTENSET_ADDRESS_Msk,    /**< Interrupt on ADDRESS event. */
    NRF_RADIO_INT_PAYLOAD_MASK    = RADIO_INTENSET_PAYLOAD_Msk,    /**< Interrupt on PAYLOAD event. */
    NRF_RADIO_INT_END_MASK        = RADIO_INTENSET_END_Msk,        /**< Interrupt on END event. */
    NRF_RADIO_INT_DISABLED_MASK   = RADIO_INTENSET_DISABLED_Msk,   /**< Interrupt on DISABLED event. */
    NRF_RADIO_INT_DEVMATCH_MASK   = RADIO_INTENSET_DEVMATCH_Msk,   /**< Interrupt on DEVMATCH event. */
    NRF_RADIO_INT_DEVMISS_MASK    = RADIO_INTENSET_DEVMISS_Msk,    /**< Interrupt on DEVMISS event. */
    NRF_RADIO_INT_RSSIEND_MASK    = RADIO_INTENSET_RSSIEND_Msk,    /**< Interrupt on RSSIEND event. */
    NRF_RADIO_INT_BCMATCH_MASK    = RADIO_INTENSET_BCMATCH_Msk,    /**< Interrupt on BCMATCH event. */
#if defined(RADIO_INTENSET_CRCOK_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_CRCOK_MASK      = RADIO_INTENSET_CRCOK_Msk,      /**< Interrupt on CRCOK event. */
#endif
#if defined(RADIO_INTENSET_CRCERROR_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_CRCERROR_MASK   = RADIO_INTENSET_CRCERROR_Msk,   /**< Interrupt on CRCERROR event. */
#endif
#if defined(RADIO_INTENSET_FRAMESTART_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_FRAMESTART_MASK = RADIO_INTENSET_FRAMESTART_Msk, /**< Interrupt on FRAMESTART event. */
#endif
#if defined(RADIO_INTENSET_EDEND_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_EDEND_MASK      = RADIO_INTENSET_EDEND_Msk,      /**< Interrupt on EDEND event. */
#endif
#if defined(RADIO_INTENSET_EDSTOPPED_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_EDSTOPPED_MASK  = RADIO_INTENSET_EDSTOPPED_Msk,  /**< Interrupt on EDSTOPPED event. */
#endif
#if defined(RADIO_INTENSET_CCAIDLE_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_CCAIDLE_MASK    = RADIO_INTENSET_CCAIDLE_Msk,    /**< Interrupt on CCAIDLE event. */
#endif
#if defined(RADIO_INTENSET_CCABUSY_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_CCABUSY_MASK    = RADIO_INTENSET_CCABUSY_Msk,    /**< Interrupt on CCABUSY event. */
#endif
#if defined(RADIO_INTENSET_CCASTOPPED_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_CCASTOPPED_MASK = RADIO_INTENSET_CCASTOPPED_Msk, /**< Interrupt on CCASTOPPED event. */
#endif
#if defined(RADIO_INTENSET_RATEBOOST_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_RATEBOOST_MASK  = RADIO_INTENSET_RATEBOOST_Msk,  /**< Interrupt on RATEBOOST event. */
#endif
#if defined(RADIO_INTENSET_TXREADY_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_TXREADY_MASK    = RADIO_INTENSET_TXREADY_Msk,    /**< Interrupt on TXREADY event. */
#endif
#if defined(RADIO_INTENSET_RXREADY_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_RXREADY_MASK    = RADIO_INTENSET_RXREADY_Msk,    /**< Interrupt on RXREADY event. */
#endif
#if defined(RADIO_INTENSET_MHRMATCH_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_MHRMATCH_MASK   = RADIO_INTENSET_MHRMATCH_Msk,   /**< Interrupt on MHRMATCH event. */
#endif
#if defined(RADIO_INTENSET_PHYEND_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_INT_PHYEND_MASK     = RADIO_INTENSET_PHYEND_Msk,     /**< Interrupt on PHYEND event. */
#endif
} nrf_radio_int_mask_t;

/**
 * @brief RADIO shortcuts.
 */
typedef enum
{
    NRF_RADIO_SHORT_READY_START_MASK        = RADIO_SHORTS_READY_START_Msk,        /**< Shortcut between READY event and START task. */
    NRF_RADIO_SHORT_END_DISABLE_MASK        = RADIO_SHORTS_END_DISABLE_Msk,        /**< Shortcut between END event and DISABLE task. */
    NRF_RADIO_SHORT_DISABLED_TXEN_MASK      = RADIO_SHORTS_DISABLED_TXEN_Msk,      /**< Shortcut between DISABLED event and TXEN task. */
    NRF_RADIO_SHORT_DISABLED_RXEN_MASK      = RADIO_SHORTS_DISABLED_RXEN_Msk,      /**< Shortcut between DISABLED event and RXEN task. */
    NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK  = RADIO_SHORTS_ADDRESS_RSSISTART_Msk,  /**< Shortcut between ADDRESS event and RSSISTART task. */
    NRF_RADIO_SHORT_END_START_MASK          = RADIO_SHORTS_END_START_Msk,          /**< Shortcut between END event and START task. */
    NRF_RADIO_SHORT_ADDRESS_BCSTART_MASK    = RADIO_SHORTS_ADDRESS_BCSTART_Msk,    /**< Shortcut between ADDRESS event and BCSTART task. */
    NRF_RADIO_SHORT_DISABLED_RSSISTOP_MASK  = RADIO_SHORTS_DISABLED_RSSISTOP_Msk,  /**< Shortcut between DISABLED event and RSSISTOP task. */
#if defined(RADIO_SHORTS_RXREADY_CCASTART_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_RXREADY_CCASTART_MASK   = RADIO_SHORTS_RXREADY_CCASTART_Msk,   /**< Shortcut between RXREADY event and CCASTART task. */
#endif
#if defined(RADIO_SHORTS_CCAIDLE_TXEN_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_CCAIDLE_TXEN_MASK       = RADIO_SHORTS_CCAIDLE_TXEN_Msk,       /**< Shortcut between CCAIDLE event and TXEN task. */
#endif
#if defined(RADIO_SHORTS_CCABUSY_DISABLE_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_CCABUSY_DISABLE_MASK    = RADIO_SHORTS_CCABUSY_DISABLE_Msk,    /**< Shortcut between CCABUSY event and DISABLE task. */
#endif
#if defined(RADIO_SHORTS_FRAMESTART_BCSTART_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_FRAMESTART_BCSTART_MASK = RADIO_SHORTS_FRAMESTART_BCSTART_Msk, /**< Shortcut between FRAMESTART event and BCSTART task. */
#endif
#if defined(RADIO_SHORTS_READY_EDSTART_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_READY_EDSTART_MASK      = RADIO_SHORTS_READY_EDSTART_Msk,      /**< Shortcut between READY event and EDSTART task. */
#endif
#if defined(RADIO_SHORTS_EDEND_DISABLE_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_EDEND_DISABLE_MASK      = RADIO_SHORTS_EDEND_DISABLE_Msk,      /**< Shortcut between EDEND event and DISABLE task. */
#endif
#if defined(RADIO_SHORTS_CCAIDLE_STOP_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_CCAIDLE_STOP_MASK       = RADIO_SHORTS_CCAIDLE_STOP_Msk,       /**< Shortcut between CCAIDLE event and STOP task. */
#endif
#if defined(RADIO_SHORTS_TXREADY_START_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_TXREADY_START_MASK      = RADIO_SHORTS_TXREADY_START_Msk,      /**< Shortcut between TXREADY event and START task. */
#endif
#if defined(RADIO_SHORTS_RXREADY_START_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_RXREADY_START_MASK      = RADIO_SHORTS_RXREADY_START_Msk,      /**< Shortcut between RXREADY event and START task. */
#endif
#if defined(RADIO_SHORTS_PHYEND_DISABLE_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_PHYEND_DISABLE_MASK     = RADIO_SHORTS_PHYEND_DISABLE_Msk,     /**< Shortcut between PHYEND event and DISABLE task. */
#endif
#if defined(RADIO_SHORTS_PHYEND_START_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_SHORT_PHYEND_START_MASK       = RADIO_SHORTS_PHYEND_START_Msk,       /**< Shortcut between PHYEND event and START task. */
#endif
} nrf_radio_short_mask_t;

#if defined(RADIO_CCACTRL_CCAMODE_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief RADIO Clear Channel Assessment modes.
 */
typedef enum
{
    NRF_RADIO_CCA_MODE_ED             = RADIO_CCACTRL_CCAMODE_EdMode,           /**< Energy Above Threshold. Will report busy whenever energy is detected above set threshold. */
    NRF_RADIO_CCA_MODE_CARRIER        = RADIO_CCACTRL_CCAMODE_CarrierMode,      /**< Carrier Seen. Will report busy whenever compliant IEEE 802.15.4 signal is seen. */
    NRF_RADIO_CCA_MODE_CARRIER_AND_ED = RADIO_CCACTRL_CCAMODE_CarrierAndEdMode, /**< Energy Above Threshold AND Carrier Seen. */
    NRF_RADIO_CCA_MODE_CARRIER_OR_ED  = RADIO_CCACTRL_CCAMODE_CarrierOrEdMode,  /**< Energy Above Threshold OR Carrier Seen. */
    NRF_RADIO_CCA_MODE_ED_TEST1       = RADIO_CCACTRL_CCAMODE_EdModeTest1,      /**< Energy Above Threshold test mode that will abort when first ED measurement over threshold is seen. No averaging. */
} nrf_radio_cca_mode_t;
#endif // defined(RADIO_CCACTRL_CCAMODE_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Types of RADIO States.
 */
typedef enum
{
    NRF_RADIO_STATE_DISABLED  = RADIO_STATE_STATE_Disabled,  /**< No operations are going on inside the radio and the power consumption is at a minimum. */
    NRF_RADIO_STATE_RXRU      = RADIO_STATE_STATE_RxRu,      /**< The radio is ramping up and preparing for reception. */
    NRF_RADIO_STATE_RXIDLE    = RADIO_STATE_STATE_RxIdle,    /**< The radio is ready for reception to start. */
    NRF_RADIO_STATE_RX        = RADIO_STATE_STATE_Rx,        /**< Reception has been started. */
    NRF_RADIO_STATE_RXDISABLE = RADIO_STATE_STATE_RxDisable, /**< The radio is disabling the receiver. */
    NRF_RADIO_STATE_TXRU      = RADIO_STATE_STATE_TxRu,      /**< The radio is ramping up and preparing for transmission. */
    NRF_RADIO_STATE_TXIDLE    = RADIO_STATE_STATE_TxIdle,    /**< The radio is ready for transmission to start. */
    NRF_RADIO_STATE_TX        = RADIO_STATE_STATE_Tx,        /**< The radio is transmitting a packet. */
    NRF_RADIO_STATE_TXDISABLE = RADIO_STATE_STATE_TxDisable, /**< The radio is disabling the transmitter. */
} nrf_radio_state_t;

/**
 * @brief Types of RADIO TX power.
 */
typedef enum
{
#if defined(RADIO_TXPOWER_TXPOWER_Pos8dBm) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TXPOWER_POS8DBM  = RADIO_TXPOWER_TXPOWER_Pos8dBm,  /**< 8 dBm. */
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos7dBm) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TXPOWER_POS7DBM  = RADIO_TXPOWER_TXPOWER_Pos7dBm,  /**< 7 dBm. */
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos6dBm) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TXPOWER_POS6DBM  = RADIO_TXPOWER_TXPOWER_Pos6dBm,  /**< 6 dBm. */
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos5dBm) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TXPOWER_POS5DBM  = RADIO_TXPOWER_TXPOWER_Pos5dBm,  /**< 5 dBm. */
#endif
    NRF_RADIO_TXPOWER_POS4DBM  = RADIO_TXPOWER_TXPOWER_Pos4dBm,  /**< 4 dBm. */
#if defined(RADIO_TXPOWER_TXPOWER_Pos3dBm) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TXPOWER_POS3DBM  = RADIO_TXPOWER_TXPOWER_Pos3dBm,  /**< 3 dBm. */
#endif
#if defined(RADIO_TXPOWER_TXPOWER_Pos2dBm) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TXPOWER_POS2DBM  = RADIO_TXPOWER_TXPOWER_Pos2dBm,  /**< 2 dBm. */
#endif
    NRF_RADIO_TXPOWER_0DBM     = RADIO_TXPOWER_TXPOWER_0dBm,     /**< 0 dBm. */
    NRF_RADIO_TXPOWER_NEG4DBM  = RADIO_TXPOWER_TXPOWER_Neg4dBm,  /**< -4 dBm. */
    NRF_RADIO_TXPOWER_NEG8DBM  = RADIO_TXPOWER_TXPOWER_Neg8dBm,  /**< -8 dBm. */
    NRF_RADIO_TXPOWER_NEG12DBM = RADIO_TXPOWER_TXPOWER_Neg12dBm, /**< -12 dBm. */
    NRF_RADIO_TXPOWER_NEG16DBM = RADIO_TXPOWER_TXPOWER_Neg16dBm, /**< -16 dBm. */
    NRF_RADIO_TXPOWER_NEG20DBM = RADIO_TXPOWER_TXPOWER_Neg20dBm, /**< -20 dBm. */
    NRF_RADIO_TXPOWER_NEG30DBM = RADIO_TXPOWER_TXPOWER_Neg30dBm, /**< -30 dBm. */
#if defined(RADIO_TXPOWER_TXPOWER_Neg40dBm) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_TXPOWER_NEG40DBM = RADIO_TXPOWER_TXPOWER_Neg40dBm, /**< -40 dBm. */
#endif
} nrf_radio_txpower_t;

/**
 * @brief Types of RADIO modes (data rate and modulation).
 */
typedef enum
{
    NRF_RADIO_MODE_NRF_1MBIT          = RADIO_MODE_MODE_Nrf_1Mbit,          /**< 1Mbit/s Nordic proprietary radio mode. */
    NRF_RADIO_MODE_NRF_2MBIT          = RADIO_MODE_MODE_Nrf_2Mbit,          /**< 2Mbit/s Nordic proprietary radio mode. */
#if defined(RADIO_MODE_MODE_Nrf_250Kbit) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_MODE_NRF_250KBIT        = RADIO_MODE_MODE_Nrf_250Kbit,        /**< 250Kbit/s Nordic proprietary radio mode. */
#endif
    NRF_RADIO_MODE_BLE_1MBIT          = RADIO_MODE_MODE_Ble_1Mbit,          /**< 1 Mbit/s Bluetooth Low Energy. */
#if defined(RADIO_MODE_MODE_Ble_2Mbit) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_MODE_BLE_2MBIT          = RADIO_MODE_MODE_Ble_2Mbit,          /**< 2 Mbit/s Bluetooth Low Energy. */
#endif
#if defined(RADIO_MODE_MODE_Ble_LR125Kbit) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_MODE_BLE_LR125KBIT      = RADIO_MODE_MODE_Ble_LR125Kbit,      /*!< Bluetooth Low Energy Long range 125 kbit/s TX, 125 kbit/s and 500 kbit/s RX */
#endif
#if defined(RADIO_MODE_MODE_Ble_LR500Kbit) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_MODE_BLE_LR500KBIT      = RADIO_MODE_MODE_Ble_LR500Kbit,      /*!< Bluetooth Low Energy Long range 500 kbit/s TX, 125 kbit/s and 500 kbit/s RX */
#endif
#if defined(RADIO_MODE_MODE_Ieee802154_250Kbit) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_MODE_IEEE802154_250KBIT = RADIO_MODE_MODE_Ieee802154_250Kbit, /**< IEEE 802.15.4-2006 250 kbit/s. */
#endif
} nrf_radio_mode_t;

#if defined(RADIO_PCNF0_PLEN_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Types of preamble length.
 */
typedef enum
{
    NRF_RADIO_PREAMBLE_LENGTH_8BIT       = RADIO_PCNF0_PLEN_8bit,      /**< 8-bit preamble. */
    NRF_RADIO_PREAMBLE_LENGTH_16BIT      = RADIO_PCNF0_PLEN_16bit,     /**< 16-bit preamble. */
#if defined(RADIO_PCNF0_PLEN_32bitZero) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_PREAMBLE_LENGTH_32BIT_ZERO = RADIO_PCNF0_PLEN_32bitZero, /**< 32-bit zero preamble used for IEEE 802.15.4. */
#endif
#if defined(RADIO_PCNF0_PLEN_LongRange) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_PREAMBLE_LENGTH_LONG_RANGE = RADIO_PCNF0_PLEN_LongRange, /**< Preamble - used for BTLE Long Range. */
#endif
} nrf_radio_preamble_length_t;
#endif // defined(RADIO_PCNF0_PLEN_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Types of CRC calculatons regarding address.
 */
typedef enum
{
    NRF_RADIO_CRC_ADDR_INCLUDE    = RADIO_CRCCNF_SKIPADDR_Include,    /**< CRC calculation includes address field. */
    NRF_RADIO_CRC_ADDR_SKIP       = RADIO_CRCCNF_SKIPADDR_Skip,       /**< CRC calculation does not include address field. */
#if defined(RADIO_CRCCNF_SKIPADDR_Ieee802154) || defined(__NRFX_DOXYGEN__)
    NRF_RADIO_CRC_ADDR_IEEE802154 = RADIO_CRCCNF_SKIPADDR_Ieee802154, /**< CRC calculation as per 802.15.4 standard. */
#endif
} nrf_radio_crc_addr_t;

/**
 * @brief Packet configuration.
 */
typedef struct
{
    uint8_t lflen;                    /**< Length on air of LENGTH field in number of bits. */
    uint8_t s0len;                    /**< Length on air of S0 field in number of bytes. */
    uint8_t s1len;                    /**< Length on air of S1 field in number of bits. */
#if defined(RADIO_PCNF0_S1INCL_Msk) || defined(__NRFX_DOXYGEN__)
    bool s1incl;                      /**< Include or exclude S1 field in RAM. */
#endif
#if defined(RADIO_PCNF0_CILEN_Msk) || defined(__NRFX_DOXYGEN__)
    uint8_t cilen;                    /**< Length of code indicator - long range. */
#endif
#if defined(RADIO_PCNF0_PLEN_Msk) || defined(__NRFX_DOXYGEN__)
    nrf_radio_preamble_length_t plen; /**< Length of preamble on air. Decision point: TASKS_START task. */
#endif
#if defined(RADIO_PCNF0_CRCINC_Msk) || defined(__NRFX_DOXYGEN__)
    bool crcinc;                      /**< Indicates if LENGTH field contains CRC or not. */
#endif
#if defined(RADIO_PCNF0_TERMLEN_Msk) || defined(__NRFX_DOXYGEN__)
    uint8_t termlen;                  /**< Length of TERM field in Long Range operation. */
#endif
    uint8_t maxlen;                   /**< Maximum length of packet payload. */
    uint8_t statlen;                  /**< Static length in number of bytes. */
    uint8_t balen;                    /**< Base address length in number of bytes. */
    bool big_endian;                  /**< On air endianness of packet. */
    bool whiteen;                     /**< Enable or disable packet whitening. */
} nrf_radio_packet_conf_t;

/**
 * @brief Function for activating a specific RADIO task.
 *
 * @param[in] radio_task Task to activate.
 */
__STATIC_INLINE void nrf_radio_task_trigger(nrf_radio_task_t radio_task);

/**
 * @brief Function for getting the address of a specific RADIO task register.
 *
 * This function can be used by the PPI module.
 *
 * @param[in] radio_task Requested task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t nrf_radio_task_address_get(nrf_radio_task_t radio_task);

/**
 * @brief Function for clearing a specific RADIO event.
 *
 * @param[in] radio_event Event to clean.
 */
__STATIC_INLINE void nrf_radio_event_clear(nrf_radio_event_t radio_event);

/**
 * @brief Function for checking the state of a specific RADIO event.
 *
 * @param[in] radio_event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_radio_event_check(nrf_radio_event_t radio_event);

/**
 * @brief Function for getting the address of a specific RADIO event register.
 *
 * This function can be used by the PPI module.
 *
 * @param[in] radio_event Requested Event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t nrf_radio_event_address_get(nrf_radio_event_t radio_event);

/**
 * @brief Function for enabling specified RADIO shortcuts.
 *
 * @param[in] radio_shorts_mask Mask of shortcuts.
 *
 */
__STATIC_INLINE void nrf_radio_shorts_enable(uint32_t radio_shorts_mask);

/**
 * @brief Function for disabling specified RADIO shortcuts.
 *
 * @param[in] radio_shorts_mask Mask of shortcuts.
 */
__STATIC_INLINE void nrf_radio_shorts_disable(uint32_t radio_shorts_mask);

/**
 * @brief Function for setting the configuration of RADIO shortcuts.
 *
 * @param[in] radio_shorts_mask Shortcuts configuration to set.
 */
__STATIC_INLINE void nrf_radio_shorts_set(uint32_t radio_shorts_mask);

/**
 * @brief Function for getting the configuration of RADIO shortcuts.
 *
 * @return Mask of currently enabled shortcuts.
 */
__STATIC_INLINE uint32_t nrf_radio_shorts_get(void);

/**
 * @brief Function for enabling specified RADIO interrupts.
 *
 * @param[in] radio_int_mask Mask of interrupts.
 */
__STATIC_INLINE void nrf_radio_int_enable(uint32_t radio_int_mask);

/**
 * @brief Function for disabling specified RADIO interrupts.
 *
 * @param[in] radio_int_mask Mask of interrupts.
 */
__STATIC_INLINE void nrf_radio_int_disable(uint32_t radio_int_mask);

/**
 * @brief Function for getting the state of a specific interrupt.
 *
 * @param[in] radio_int_mask Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_radio_int_enable_check(nrf_radio_int_mask_t radio_int_mask);

/**
 * @brief Function for getting CRC status of last received packet.
 *
 * @retval true  If the packet was received without CRC error .
 * @retval false If the packet was received with CRC error.
 */
__STATIC_INLINE bool nrf_radio_crc_status_check(void);

/**
 * @brief Function for getting the received address.
 *
 * @return Received address.
 */
__STATIC_INLINE uint8_t nrf_radio_rxmatch_get(void);

/**
 * @brief Function for getting CRC field of the last received packet.
 *
 * @return CRC field of previously received packet.
 */
__STATIC_INLINE uint32_t nrf_radio_rxcrc_get(void);

/**
 * @brief Function for getting the device address match index.
 *
 * @return Device adress match index.
 */
__STATIC_INLINE uint8_t nrf_radio_dai_get(void);

#if defined(RADIO_PDUSTAT_PDUSTAT_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for getting status on payload length.
 *
 * @retval 0 If the payload is lesser than PCNF1.MAXLEN.
 * @retval 1 If the payload is greater than PCNF1.MAXLEN.
 */
__STATIC_INLINE uint8_t nrf_radio_pdustat_get(void);

/**
 * @brief Function for getting status on what rate packet is received with in Long Range.
 *
 * @retval 0 If the frame is received at 125kbps.
 * @retval 1 If the frame is received at 500kbps.
 */
__STATIC_INLINE uint8_t nrf_radio_cistat_get(void);
#endif // defined(RADIO_PDUSTAT_PDUSTAT_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for setting packet pointer to given location in memory.
 *
 * @param[in] p_packet Packet pointer.
 */
__STATIC_INLINE void nrf_radio_packetptr_set(const void * p_packet);

/**
 * @brief Function for getting packet pointer.
 *
 * @return Pointer to tx or rx packet buffer.
 */
__STATIC_INLINE void * nrf_radio_packetptr_get(void);

/**
 * @brief Function for setting the radio frequency.
 *
 * @param[in] radio_frequency Frequency in MHz.
 */
__STATIC_INLINE void nrf_radio_frequency_set(uint16_t radio_frequency);

/**
 * @brief Function for getting the radio frequency.
 *
 * @return Frequency in MHz.
 */
__STATIC_INLINE uint16_t nrf_radio_frequency_get(void);

/**
 * @brief Function for setting the radio transmit power.
 *
 * @param[in] tx_power Transmit power of the radio [dBm].
 */
__STATIC_INLINE void nrf_radio_txpower_set(nrf_radio_txpower_t tx_power);

/**
 * @brief Function for getting the radio transmit power.
 *
 * @return Transmit power of the radio.
 */
__STATIC_INLINE nrf_radio_txpower_t nrf_radio_txpower_get(void);

/**
 * @brief Function for setting the radio data rate and modulation settings.
 *
 * @param[in] radio_mode Radio data rate and modulation.
 */
__STATIC_INLINE void nrf_radio_mode_set(nrf_radio_mode_t radio_mode);

/**
 * @brief Function for getting Radio data rate and modulation settings.
 *
 * @return Radio data rate and modulation.
 */
__STATIC_INLINE nrf_radio_mode_t nrf_radio_mode_get(void);

/**
 * @brief Function for setting the packet configuration.
 *
 * @param[in] p_config Pointer to the structure with packet configuration.
 */
__STATIC_INLINE void nrf_radio_packet_configure(const nrf_radio_packet_conf_t * p_config);

/**
 * @brief Function for setting the base address 0.
 *
 * @param address Base address 0 value.
 */
__STATIC_INLINE void nrf_radio_base0_set(uint32_t address);

/**
 * @brief Function for getting the base address 0.
 *
 * @return Base address 0.
 */
__STATIC_INLINE uint32_t nrf_radio_base0_get(void);

/**
 * @brief Function for setting Base address 1.
 *
 * @param address Base address 1 value.
 */
__STATIC_INLINE void nrf_radio_base1_set(uint32_t address);

/**
 * @brief Function for getting base address 1.
 *
 * @return Base address 1.
 */
__STATIC_INLINE uint32_t nrf_radio_base1_get(void);

/**
 * @brief Function for setting prefixes bytes for logical addresses 0-3.
 *
 * @param prefixes Prefixes bytes for logical addresses 0-3.
 */
__STATIC_INLINE void nrf_radio_prefix0_set(uint32_t prefixes);

/**
 * @brief Function for getting prefixes bytes for logical addresses 0-3
 *
 * @return Prefixes bytes for logical addresses 0-3
 */
__STATIC_INLINE uint32_t nrf_radio_prefix0_get(void);

/**
 * @brief Function for setting prefixes bytes for logical addresses 4-7.
 *
 * @param prefixes Prefixes bytes for logical addresses 4-7.
 */
__STATIC_INLINE void nrf_radio_prefix1_set(uint32_t prefixes);

/**
 * @brief Function for getting prefixes bytes for logical addresses 4-7
 *
 * @return Prefixes bytes for logical addresses 4-7
 */
__STATIC_INLINE uint32_t nrf_radio_prefix1_get(void);

/**
 * @brief Function for setting the transmit address.
 *
 * @param txaddress Logical address to be used when transmitting a packet.
 */
__STATIC_INLINE void nrf_radio_txaddress_set(uint8_t txaddress);

/**
 * @brief Function for getting the transmit address select.
 *
 * @return Logical address to be used when transmitting a packet.
 */
__STATIC_INLINE uint8_t nrf_radio_txaddress_get(void);

/**
 * @brief Function for for selecting the receive addresses.
 *
 * @param rxaddresses Enable or disable reception on logical address i.
 *                    Read more in the Product Specification.
 */
__STATIC_INLINE void nrf_radio_rxaddresses_set(uint8_t rxaddresses);

/**
 * @brief Function for getting receive address select.
 *
 * @return Receive address select.
 */
__STATIC_INLINE uint8_t nrf_radio_rxaddresses_get(void);

/**
 * @brief Function for configure CRC.
 *
 * @param[in] crc_length      CRC length in number of bytes [0-3].
 * @param[in] crc_address     Include or exclude packet address field out of CRC.
 * @param[in] crc_polynominal CRC polynominal to set.
 */
__STATIC_INLINE void nrf_radio_crc_configure(uint8_t              crc_length,
                                             nrf_radio_crc_addr_t crc_address,
                                             uint32_t             crc_polynominal);

/**
 * @brief Function for setting CRC initial value.
 *
 * @param crc_init_value CRC initial value
 */
__STATIC_INLINE void nrf_radio_crcinit_set(uint32_t crc_init_value);

/**
 * @brief Function for getting CRC initial value.
 *
 * @return CRC initial value.
 */
__STATIC_INLINE uint32_t nrf_radio_crcinit_get(void);

/**
 * @brief Function for setting Inter Frame Spacing interval.
 *
 * @param[in] radio_ifs Inter frame spacing interval [us].
 */
__STATIC_INLINE void nrf_radio_ifs_set(uint32_t radio_ifs);

/**
 * @brief Function for getting Inter Frame Spacing interval.
 *
 * @return Inter frame spacing interval [us].
 */
__STATIC_INLINE uint32_t nrf_radio_ifs_get(void);

/**
 * @brief Function for getting RSSI sample result.
 *
 * @note The read value is a positive value while the actual received signal
 *       is a negative value. Actual received signal strength is therefore as follows:
 *       received signal strength = - read_value dBm .
 *
 * @return RSSI sample result.
 */
__STATIC_INLINE uint8_t nrf_radio_rssi_sample_get(void);

/**
 * @brief Function for getting the current state of the radio module.
 *
 * @return Current radio state.
 */
__STATIC_INLINE nrf_radio_state_t nrf_radio_state_get(void);

/**
 * @brief Function for setting the data whitening initial value.
 *
 * @param datawhiteiv Data whitening initial value.
 */
__STATIC_INLINE void nrf_radio_datawhiteiv_set(uint8_t datawhiteiv);

/**
 * @brief Function for getting the data whitening initial value.
 *
 * @return Data whitening initial value.
 */
__STATIC_INLINE uint8_t nrf_radio_datawhiteiv_get(void);

/**
 * @brief Function for setting Bit counter compare.
 *
 * @param[in] radio_bcc Bit counter compare [bits].
 */
__STATIC_INLINE void nrf_radio_bcc_set(uint32_t radio_bcc);

/**
 * @brief Function for getting Bit counter compare.
 *
 * @return Bit counter compare.
 */
__STATIC_INLINE uint32_t nrf_radio_bcc_get(void);

/**
 * @brief Function for setting Device address base segment.
 *
 * @param dab_value Particular base segment value.
 * @param segment   Index of the particular Device address base segment register.
 */
__STATIC_INLINE void nrf_radio_dab_set(uint32_t dab_value, uint8_t segment);

/**
 * @brief Function for getting Device address base segment.
 *
 * @param segment Number of the Device address base segment.
 *
 * @return Particular segment of the Device address base.
 */
__STATIC_INLINE uint32_t nrf_radio_dab_get(uint8_t segment);

/**
 * @brief Function for setting device address prefix.
 *
 * @param dap_value    Particular device address prefix value.
 * @param prefix_index Index of the particular device address prefix register.
 */
__STATIC_INLINE void nrf_radio_dap_set(uint16_t dap_value, uint8_t prefix_index);

/**
 * @brief Function for getting Device address prefix.
 *
 * @param prefix_index Number of the Device address prefix segment.
 *
 * @return Particular segment of the Device address prefix.
 */
__STATIC_INLINE uint32_t nrf_radio_dap_get(uint8_t prefix_index);

/**
 * @brief Function for setting device address match configuration.
 *
 * @note Read more about configuring device address match in the Product Specification.
 *
 * @param ena   Device address matching bitmask.
 * @param txadd TxAdd bitmask.
 */
__STATIC_INLINE void nrf_radio_dacnf_set(uint8_t ena, uint8_t txadd);

/**
 * @brief Function for getting ENA field of the Device address match configuration register.
 *
 * @return ENA field of the Device address match configuration register.
 */
__STATIC_INLINE uint8_t nrf_radio_dacnf_ena_get(void);

/**
 * @brief Function for getting TXADD field of the Device address match configuration register.
 *
 * @return TXADD field of the Device address match configuration register.
 */
__STATIC_INLINE uint8_t nrf_radio_dacnf_txadd_get(void);

#if defined(RADIO_INTENSET_MHRMATCH_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting MAC Header Match Unit search pattern configuration.
 *
 * @param[in] radio_mhmu_search_pattern Search Pattern Configuration.
 */
__STATIC_INLINE void nrf_radio_mhmu_search_pattern_set(uint32_t radio_mhmu_search_pattern);

/**
 * @brief Function for getting MAC Header Match Unit search pattern configuration.
 *
 * @return Search Pattern Configuration.
 */
__STATIC_INLINE uint32_t nrf_radio_mhmu_search_pattern_get(void);

/**
 * @brief Function for setting MAC Header Match Unit pattern mask configuration.
 *
 * @param[in] radio_mhmu_pattern_mask Pattern mask.
 */
__STATIC_INLINE void nrf_radio_mhmu_pattern_mask_set(uint32_t radio_mhmu_pattern_mask);

/**
 * @brief Function for getting MAC Header Match Unit pattern mask configuration.
 *
 * @return Pattern mask.
 */
__STATIC_INLINE uint32_t nrf_radio_mhmu_pattern_mask_get(void);
#endif // defined(RADIO_INTENSET_MHRMATCH_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(RADIO_MODECNF0_RU_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting Radio mode configuration register 0.
 *
 * @param fast_ramp_up Use fast radio ramp-up time
 * @param default_tx   Default TX value during inactivity.
 */
__STATIC_INLINE void nrf_radio_modecnf0_set(bool fast_ramp_up, uint8_t default_tx);

/**
 * @brief Function for getting ramp-up time configuration of the Radio mode configuration register 0.
 *
 * @retval true  If the ramp-up time is set to fast.
 * @retval false If the ramp-up time is set to default.
 */
__STATIC_INLINE bool nrf_radio_modecnf0_ru_get(void);

/**
 * @brief Function for getting default TX value of the Radio mode configuration register 0.
 *
 * @return Default TX value.
 */
__STATIC_INLINE uint8_t nrf_radio_modecnf0_dtx_get(void);
#endif // defined(RADIO_MODECNF0_RU_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(RADIO_SFD_SFD_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting IEEE 802.15.4 start of frame delimiter.
 *
 * @param sfd IEEE 802.15.4 start of frame delimiter.
 */
__STATIC_INLINE void nrf_radio_sfd_set(uint8_t sfd);

/**
 * @brief Function for getting IEEE 802.15.4 start of frame delimiter.
 *
 * @return IEEE 802.15.4 start of frame delimiter.
 */
__STATIC_INLINE uint8_t nrf_radio_sfd_get(void);
#endif // defined(RADIO_SFD_SFD_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(RADIO_EDCNT_EDCNT_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting number of iterations to perform ED scan.
 *
 * @param[in] ed_loop_count Number of iterations during ED procedure.
 */
__STATIC_INLINE void nrf_radio_ed_loop_count_set(uint32_t ed_loop_count);
#endif // defined(RADIO_EDCNT_EDCNT_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(RADIO_EDSAMPLE_EDLVL_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for getting Energy Detection level.
 *
 * @return IEEE 802.15.4 energy detect level.
 */
__STATIC_INLINE uint8_t nrf_radio_ed_sample_get(void);
#endif // defined(RADIO_EDSAMPLE_EDLVL_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(RADIO_CCACTRL_CCAMODE_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for configuring the IEEE 802.15.4 clear channel assessment.
 *
 * @param cca_mode           Mode of CCA.
 * @param cca_ed_threshold   Energy Detection threshold value.
 * @param cca_corr_threshold Correlator Busy Threshold.
 * @param cca_corr_cnt       Limit of occurances above Correlator Threshold.
 *                           When not equal to zero the correlator based
 *                           signal detect is enabled.
 */
__STATIC_INLINE void nrf_radio_cca_configure(nrf_radio_cca_mode_t cca_mode,
                                             uint8_t              cca_ed_threshold,
                                             uint8_t              cca_corr_threshold,
                                             uint8_t              cca_corr_cnt);
#endif // defined(RADIO_CCACTRL_CCAMODE_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for setting power mode of the radio peripheral.
 *
 * @param[in] radio_power If radio should be powered on.
 */
__STATIC_INLINE void nrf_radio_power_set(bool radio_power);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_radio_task_trigger(nrf_radio_task_t radio_task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_RADIO + radio_task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_radio_task_address_get(nrf_radio_task_t radio_task)
{
    return ((uint32_t)NRF_RADIO + (uint32_t)radio_task);
}

__STATIC_INLINE void nrf_radio_event_clear(nrf_radio_event_t radio_event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_RADIO + radio_event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_RADIO + radio_event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_radio_event_check(nrf_radio_event_t radio_event)
{
    return (bool) *((volatile uint32_t *)((uint8_t *)NRF_RADIO + radio_event));
}

__STATIC_INLINE uint32_t nrf_radio_event_address_get(nrf_radio_event_t radio_event)
{
    return ((uint32_t)NRF_RADIO + (uint32_t)radio_event);
}

__STATIC_INLINE void nrf_radio_shorts_enable(uint32_t radio_shorts_mask)
{
    NRF_RADIO->SHORTS |= radio_shorts_mask;
}

__STATIC_INLINE void nrf_radio_shorts_disable(uint32_t radio_shorts_mask)
{
    NRF_RADIO->SHORTS &= ~radio_shorts_mask;
}

__STATIC_INLINE void nrf_radio_shorts_set(uint32_t radio_shorts_mask)
{
    NRF_RADIO->SHORTS = radio_shorts_mask;
}

__STATIC_INLINE uint32_t nrf_radio_shorts_get(void)
{
    return NRF_RADIO->SHORTS;
}

__STATIC_INLINE void nrf_radio_int_enable(uint32_t radio_int_mask)
{
    NRF_RADIO->INTENSET = radio_int_mask;
}

__STATIC_INLINE void nrf_radio_int_disable(uint32_t radio_int_mask)
{
    NRF_RADIO->INTENCLR = radio_int_mask;
}

__STATIC_INLINE bool nrf_radio_int_enable_check(nrf_radio_int_mask_t radio_int_mask)
{
    return (bool)(NRF_RADIO->INTENSET & radio_int_mask);
}

__STATIC_INLINE bool nrf_radio_crc_status_check(void)
{
    return ((NRF_RADIO->CRCSTATUS & RADIO_CRCSTATUS_CRCSTATUS_Msk) >> RADIO_CRCSTATUS_CRCSTATUS_Pos)
             == RADIO_CRCSTATUS_CRCSTATUS_CRCOk ;
}

__STATIC_INLINE uint8_t nrf_radio_rxmatch_get(void)
{
    return (uint8_t)NRF_RADIO->RXMATCH;
}

__STATIC_INLINE uint32_t nrf_radio_rxcrc_get(void)
{
    return NRF_RADIO->RXCRC;
}

__STATIC_INLINE uint8_t nrf_radio_dai_get(void)
{
    return (uint8_t)NRF_RADIO->DAI;
}

#if defined(RADIO_PDUSTAT_PDUSTAT_Msk)
__STATIC_INLINE uint8_t nrf_radio_pdustat_get(void)
{
    return (uint8_t)(NRF_RADIO->PDUSTAT & RADIO_PDUSTAT_PDUSTAT_Msk);
}

__STATIC_INLINE uint8_t nrf_radio_cistat_get(void)
{
    return (uint8_t)((NRF_RADIO->PDUSTAT & RADIO_PDUSTAT_CISTAT_Msk) >> RADIO_PDUSTAT_CISTAT_Pos);
}
#endif // defined(RADIO_PDUSTAT_PDUSTAT_Msk)

__STATIC_INLINE void nrf_radio_packetptr_set(const void * p_packet)
{
    NRF_RADIO->PACKETPTR = (uint32_t)p_packet;
}

__STATIC_INLINE void * nrf_radio_packetptr_get(void)
{
    return (void *)NRF_RADIO->PACKETPTR;
}

__STATIC_INLINE void nrf_radio_frequency_set(uint16_t radio_frequency)
{
    NRFX_ASSERT(radio_frequency <= 2500);

#if defined(RADIO_FREQUENCY_MAP_Msk)
    NRFX_ASSERT(radio_frequency >= 2360);

    uint32_t delta;
    if (radio_frequency < 2400)
    {
        delta = ((uint32_t)(radio_frequency - 2360)) |
                (RADIO_FREQUENCY_MAP_Low << RADIO_FREQUENCY_MAP_Pos);
    }
    else
    {
        delta = ((uint32_t)(radio_frequency - 2400)) |
                (RADIO_FREQUENCY_MAP_Default << RADIO_FREQUENCY_MAP_Pos);
    }

    NRF_RADIO->FREQUENCY = delta;
#else
    NRFX_ASSERT(radio_frequency >= 2400);
    NRF_RADIO->FREQUENCY = (uint32_t)(2400 - radio_frequency);
#endif //defined(RADIO_FREQUENCY_MAP_Msk)
}

__STATIC_INLINE uint16_t nrf_radio_frequency_get(void)
{
    uint32_t freq;

#if defined(RADIO_FREQUENCY_MAP_Msk)
    if (((NRF_RADIO->FREQUENCY & RADIO_FREQUENCY_MAP_Msk) >> RADIO_FREQUENCY_MAP_Pos) ==
        RADIO_FREQUENCY_MAP_Low)
    {
        freq = 2360;
    }
    else
#endif
    {
        freq = 2400;
    }
    freq += NRF_RADIO->FREQUENCY & RADIO_FREQUENCY_FREQUENCY_Msk;

    return freq;
}

__STATIC_INLINE void nrf_radio_txpower_set(nrf_radio_txpower_t tx_power)
{
    NRF_RADIO->TXPOWER = (((uint32_t)tx_power) << RADIO_TXPOWER_TXPOWER_Pos);
}

__STATIC_INLINE nrf_radio_txpower_t nrf_radio_txpower_get(void)
{
    return (nrf_radio_txpower_t)(NRF_RADIO->TXPOWER >> RADIO_TXPOWER_TXPOWER_Pos);
}

__STATIC_INLINE void nrf_radio_mode_set(nrf_radio_mode_t radio_mode)
{
    NRF_RADIO->MODE = ((uint32_t) radio_mode << RADIO_MODE_MODE_Pos);
}

__STATIC_INLINE nrf_radio_mode_t nrf_radio_mode_get(void)
{
    return (nrf_radio_mode_t)((NRF_RADIO->MODE & RADIO_MODE_MODE_Msk) >> RADIO_MODE_MODE_Pos);
}

__STATIC_INLINE void nrf_radio_packet_configure(const nrf_radio_packet_conf_t * p_config)
{
    NRF_RADIO->PCNF0 = (((uint32_t)p_config->lflen << RADIO_PCNF0_LFLEN_Pos) |
                        ((uint32_t)p_config->s0len << RADIO_PCNF0_S0LEN_Pos) |
                        ((uint32_t)p_config->s1len << RADIO_PCNF0_S1LEN_Pos) |
#if defined(RADIO_PCNF0_S1INCL_Msk)
                        (p_config->s1incl ?
                             (RADIO_PCNF0_S1INCL_Include   << RADIO_PCNF0_S1INCL_Pos) :
                             (RADIO_PCNF0_S1INCL_Automatic << RADIO_PCNF0_S1INCL_Pos) ) |
#endif
#if defined(RADIO_PCNF0_CILEN_Msk)
                        ((uint32_t)p_config->cilen << RADIO_PCNF0_CILEN_Pos) |
#endif
#if defined(RADIO_PCNF0_PLEN_Msk)
                        ((uint32_t)p_config->plen << RADIO_PCNF0_PLEN_Pos) |
#endif
#if defined(RADIO_PCNF0_CRCINC_Msk)
                        (p_config->crcinc ?
                             (RADIO_PCNF0_CRCINC_Include << RADIO_PCNF0_CRCINC_Pos) :
                             (RADIO_PCNF0_CRCINC_Exclude << RADIO_PCNF0_CRCINC_Pos) ) |
#endif
#if defined(RADIO_PCNF0_TERMLEN_Msk)
                        ((uint32_t)p_config->termlen << RADIO_PCNF0_TERMLEN_Pos) |
#endif
                        0);

    NRF_RADIO->PCNF1 = (((uint32_t)p_config->maxlen  << RADIO_PCNF1_MAXLEN_Pos) |
                        ((uint32_t)p_config->statlen << RADIO_PCNF1_STATLEN_Pos) |
                        ((uint32_t)p_config->balen   << RADIO_PCNF1_BALEN_Pos) |
                        (p_config->big_endian ?
                             (RADIO_PCNF1_ENDIAN_Big    << RADIO_PCNF1_ENDIAN_Pos) :
                             (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos) ) |
                        (p_config->whiteen ?
                             (RADIO_PCNF1_WHITEEN_Enabled  << RADIO_PCNF1_WHITEEN_Pos) :
                             (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) ));
}

__STATIC_INLINE void nrf_radio_base0_set(uint32_t address)
{
    NRF_RADIO->BASE0 = address;
}

__STATIC_INLINE uint32_t nrf_radio_base0_get(void)
{
    return NRF_RADIO->BASE0;
}

__STATIC_INLINE void nrf_radio_base1_set(uint32_t address)
{
    NRF_RADIO->BASE1 = address;
}

__STATIC_INLINE uint32_t nrf_radio_base1_get(void)
{
    return NRF_RADIO->BASE1;
}

__STATIC_INLINE void nrf_radio_prefix0_set(uint32_t prefix0_value)
{
    NRF_RADIO->PREFIX0 = prefix0_value;
}

__STATIC_INLINE uint32_t nrf_radio_prefix0_get(void)
{
    return NRF_RADIO->PREFIX0;
}

__STATIC_INLINE void nrf_radio_prefix1_set(uint32_t prefix1_value)
{
    NRF_RADIO->PREFIX1 = prefix1_value;
}

__STATIC_INLINE uint32_t nrf_radio_prefix1_get(void)
{
    return NRF_RADIO->PREFIX1;
}

__STATIC_INLINE void nrf_radio_txaddress_set(uint8_t txaddress)
{
    NRF_RADIO->TXADDRESS = ((uint32_t)txaddress) << RADIO_TXADDRESS_TXADDRESS_Pos;
}

__STATIC_INLINE uint8_t nrf_radio_txaddress_get(void)
{
    return (uint8_t)((NRF_RADIO->TXADDRESS & RADIO_TXADDRESS_TXADDRESS_Msk) >>
                     RADIO_TXADDRESS_TXADDRESS_Pos);
}

__STATIC_INLINE void nrf_radio_rxaddresses_set(uint8_t rxaddresses)
{
    NRF_RADIO->RXADDRESSES = (uint32_t)(rxaddresses);
}

__STATIC_INLINE uint8_t nrf_radio_rxaddresses_get(void)
{
    return (uint8_t)(NRF_RADIO->RXADDRESSES);
}

__STATIC_INLINE void nrf_radio_crc_configure(uint8_t              crc_length,
                                             nrf_radio_crc_addr_t crc_address,
                                             uint32_t             crc_polynominal)
{
    NRF_RADIO->CRCCNF = ((uint32_t)crc_length  << RADIO_CRCCNF_LEN_Pos) |
                        ((uint32_t)crc_address << RADIO_CRCCNF_SKIPADDR_Pos);
    NRF_RADIO->CRCPOLY = (crc_polynominal << RADIO_CRCPOLY_CRCPOLY_Pos);
}

__STATIC_INLINE void nrf_radio_crcinit_set(uint32_t crc_init_value)
{
    NRF_RADIO->CRCINIT = crc_init_value;
}

__STATIC_INLINE uint32_t nrf_radio_crcinit_get(void)
{
    return NRF_RADIO->CRCINIT;
}

__STATIC_INLINE void nrf_radio_ifs_set(uint32_t radio_ifs)
{
    NRF_RADIO->TIFS = radio_ifs;
}

__STATIC_INLINE uint32_t nrf_radio_ifs_get(void)
{
    return NRF_RADIO->TIFS;
}

__STATIC_INLINE uint8_t nrf_radio_rssi_sample_get(void)
{
    return (uint8_t)((NRF_RADIO->RSSISAMPLE & RADIO_RSSISAMPLE_RSSISAMPLE_Msk) >>
                     RADIO_RSSISAMPLE_RSSISAMPLE_Pos);
}

__STATIC_INLINE nrf_radio_state_t nrf_radio_state_get(void)
{
    return (nrf_radio_state_t) NRF_RADIO->STATE;
}

__STATIC_INLINE void nrf_radio_datawhiteiv_set(uint8_t datawhiteiv)
{
    NRF_RADIO->DATAWHITEIV = (((uint32_t)datawhiteiv) & RADIO_DATAWHITEIV_DATAWHITEIV_Msk);
}

__STATIC_INLINE uint8_t nrf_radio_datawhiteiv_get(void)
{
    return (uint8_t)(NRF_RADIO->DATAWHITEIV & RADIO_DATAWHITEIV_DATAWHITEIV_Msk);
}

__STATIC_INLINE void nrf_radio_bcc_set(uint32_t radio_bcc)
{
    NRF_RADIO->BCC = radio_bcc;
}

__STATIC_INLINE uint32_t nrf_radio_bcc_get(void)
{
    return NRF_RADIO->BCC;
}

__STATIC_INLINE void nrf_radio_dab_set(uint32_t dab_value, uint8_t segment)
{
    NRFX_ASSERT(segment < 8);
    NRF_RADIO->DAB[segment] = dab_value;
}

__STATIC_INLINE uint32_t nrf_radio_dab_get(uint8_t segment)
{
    NRFX_ASSERT(segment < 8);
    return NRF_RADIO->DAB[segment];
}

__STATIC_INLINE void nrf_radio_dap_set(uint16_t dap_value, uint8_t prefix_index)
{
    NRFX_ASSERT(prefix_index < 8);
    NRF_RADIO->DAP[prefix_index] = (uint32_t)dap_value;
}

__STATIC_INLINE uint32_t nrf_radio_dap_get(uint8_t prefix_index)
{
    NRFX_ASSERT(prefix_index < 8);
    return NRF_RADIO->DAP[prefix_index];
}

__STATIC_INLINE void nrf_radio_dacnf_set(uint8_t ena, uint8_t txadd)
{
    NRF_RADIO->DACNF = (((uint32_t)ena   << RADIO_DACNF_ENA0_Pos) |
                        ((uint32_t)txadd << RADIO_DACNF_TXADD0_Pos));
}

__STATIC_INLINE uint8_t nrf_radio_dacnf_ena_get(void)
{
    return (NRF_RADIO->DACNF & (RADIO_DACNF_ENA0_Msk |
                                RADIO_DACNF_ENA1_Msk |
                                RADIO_DACNF_ENA2_Msk |
                                RADIO_DACNF_ENA3_Msk |
                                RADIO_DACNF_ENA4_Msk |
                                RADIO_DACNF_ENA5_Msk |
                                RADIO_DACNF_ENA6_Msk |
                                RADIO_DACNF_ENA7_Msk)) >> RADIO_DACNF_ENA0_Pos;
}

__STATIC_INLINE uint8_t nrf_radio_dacnf_txadd_get(void)
{
    return (NRF_RADIO->DACNF & (RADIO_DACNF_TXADD0_Msk |
                                RADIO_DACNF_TXADD1_Msk |
                                RADIO_DACNF_TXADD2_Msk |
                                RADIO_DACNF_TXADD3_Msk |
                                RADIO_DACNF_TXADD4_Msk |
                                RADIO_DACNF_TXADD5_Msk |
                                RADIO_DACNF_TXADD6_Msk |
                                RADIO_DACNF_TXADD7_Msk)) >> RADIO_DACNF_TXADD0_Pos;
}

#if defined(RADIO_INTENSET_MHRMATCH_Msk)
__STATIC_INLINE void nrf_radio_mhmu_search_pattern_set(uint32_t radio_mhmu_search_pattern)
{
    NRF_RADIO->MHRMATCHCONF = radio_mhmu_search_pattern;
}

__STATIC_INLINE uint32_t nrf_radio_mhmu_search_pattern_get(void)
{
    return NRF_RADIO->MHRMATCHCONF;
}

__STATIC_INLINE void nrf_radio_mhmu_pattern_mask_set(uint32_t radio_mhmu_pattern_mask)
{
    NRF_RADIO->MHRMATCHMAS = radio_mhmu_pattern_mask;
}

__STATIC_INLINE uint32_t nrf_radio_mhmu_pattern_mask_get(void)
{
    return NRF_RADIO->MHRMATCHMAS;
}
#endif // defined(RADIO_INTENSET_MHRMATCH_Msk)

#if defined(RADIO_MODECNF0_RU_Msk)
__STATIC_INLINE void nrf_radio_modecnf0_set(bool fast_ramp_up, uint8_t default_tx)
{
    NRF_RADIO->MODECNF0 = (fast_ramp_up ? (RADIO_MODECNF0_RU_Fast    << RADIO_MODECNF0_RU_Pos) :
                                          (RADIO_MODECNF0_RU_Default << RADIO_MODECNF0_RU_Pos) ) |
                          (((uint32_t)default_tx) << RADIO_MODECNF0_DTX_Pos);
}

__STATIC_INLINE bool nrf_radio_modecnf0_ru_get(void)
{
    return ((NRF_RADIO->MODECNF0 & RADIO_MODECNF0_RU_Msk) >> RADIO_MODECNF0_RU_Pos) ==
            RADIO_MODECNF0_RU_Fast;
}

__STATIC_INLINE uint8_t nrf_radio_modecnf0_dtx_get(void)
{
    return (uint8_t)((NRF_RADIO->MODECNF0 & RADIO_MODECNF0_DTX_Msk) >> RADIO_MODECNF0_DTX_Pos);
}
#endif // defined(RADIO_MODECNF0_RU_Msk)

#if defined(RADIO_SFD_SFD_Msk)
__STATIC_INLINE void nrf_radio_sfd_set(uint8_t sfd)
{
    NRF_RADIO->SFD = ((uint32_t)sfd) << RADIO_SFD_SFD_Pos;
}

__STATIC_INLINE uint8_t nrf_radio_sfd_get(void)
{
    return (uint8_t)((NRF_RADIO->SFD & RADIO_SFD_SFD_Msk) >> RADIO_SFD_SFD_Pos);
}
#endif // defined(RADIO_SFD_SFD_Msk)

#if defined(RADIO_EDCNT_EDCNT_Msk)
__STATIC_INLINE void nrf_radio_ed_loop_count_set(uint32_t ed_loop_count)
{
    NRF_RADIO->EDCNT = (ed_loop_count & RADIO_EDCNT_EDCNT_Msk);
}
#endif

#if defined(RADIO_EDSAMPLE_EDLVL_Msk)
__STATIC_INLINE uint8_t nrf_radio_ed_sample_get(void)
{
    return (uint8_t) NRF_RADIO->EDSAMPLE;
}
#endif

#if defined(RADIO_CCACTRL_CCAMODE_Msk)

__STATIC_INLINE void nrf_radio_cca_configure(nrf_radio_cca_mode_t cca_mode,
                                             uint8_t              cca_ed_threshold,
                                             uint8_t              cca_corr_threshold,
                                             uint8_t              cca_corr_cnt)
{
    NRF_RADIO->CCACTRL = (((uint32_t)cca_mode           << RADIO_CCACTRL_CCAMODE_Pos) |
                          ((uint32_t)cca_ed_threshold   << RADIO_CCACTRL_CCAEDTHRES_Pos) |
                          ((uint32_t)cca_corr_threshold << RADIO_CCACTRL_CCACORRTHRES_Pos) |
                          ((uint32_t)cca_corr_cnt       << RADIO_CCACTRL_CCACORRCNT_Pos));
}
#endif

__STATIC_INLINE void nrf_radio_power_set(bool radio_power)
{
    NRF_RADIO->POWER = (uint32_t) radio_power;
}
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_RADIO_H__
