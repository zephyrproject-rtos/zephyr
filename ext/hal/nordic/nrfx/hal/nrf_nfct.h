/*
 * Copyright (c) 2018 - 2019, Nordic Semiconductor ASA
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

#ifndef NRF_NFCT_H__
#define NRF_NFCT_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_nfct_hal NFCT HAL
 * @{
 * @ingroup nrf_nfct
 *
 * @brief Hardware access layer (HAL) for the Near Field Communication Tag (NFCT) peripheral.
 */

#define NRF_NFCT_CRC_SIZE 2                 /**< CRC size in bytes. */
#define NRF_NFCT_DISABLE_ALL_INT 0xFFFFFFFF /**< Value to disable all interrupts. */

/** @brief NFCT tasks. */
typedef enum
{
    NRF_NFCT_TASK_ACTIVATE     = offsetof(NRF_NFCT_Type, TASKS_ACTIVATE),     /**< Activate the NFCT peripheral for the incoming and outgoing frames, change state to activated. */
    NRF_NFCT_TASK_DISABLE      = offsetof(NRF_NFCT_Type, TASKS_DISABLE),      /**< Disable the NFCT peripheral. */
    NRF_NFCT_TASK_SENSE        = offsetof(NRF_NFCT_Type, TASKS_SENSE),        /**< Enable the NFC sense field mode, change state to sense mode. */
    NRF_NFCT_TASK_STARTTX      = offsetof(NRF_NFCT_Type, TASKS_STARTTX),      /**< Start the transmission of an outgoing frame, change state to transmit. */
    NRF_NFCT_TASK_ENABLERXDATA = offsetof(NRF_NFCT_Type, TASKS_ENABLERXDATA), /**< Initialize EasyDMA for receive. */
    NRF_NFCT_TASK_GOIDLE       = offsetof(NRF_NFCT_Type, TASKS_GOIDLE),       /**< Force state machine to the IDLE state. */
    NRF_NFCT_TASK_GOSLEEP      = offsetof(NRF_NFCT_Type, TASKS_GOSLEEP),      /**< Force state machine to the SLEEP_A state. */
} nrf_nfct_task_t;

/** @brief NFCT events. */
typedef enum
{
    NRF_NFCT_EVENT_READY             = offsetof(NRF_NFCT_Type, EVENTS_READY),             /**< The NFCT peripheral is ready to receive and send frames. */
    NRF_NFCT_EVENT_FIELDDETECTED     = offsetof(NRF_NFCT_Type, EVENTS_FIELDDETECTED),     /**< Remote NFC field is detected. */
    NRF_NFCT_EVENT_FIELDLOST         = offsetof(NRF_NFCT_Type, EVENTS_FIELDLOST),         /**< Remote NFC field is lost. */
    NRF_NFCT_EVENT_TXFRAMESTART      = offsetof(NRF_NFCT_Type, EVENTS_TXFRAMESTART),      /**< The start of the first symbol of a transmitted frame. */
    NRF_NFCT_EVENT_TXFRAMEEND        = offsetof(NRF_NFCT_Type, EVENTS_TXFRAMEEND),        /**< The end of the last transmitted on-air symbol of a frame. */
    NRF_NFCT_EVENT_RXFRAMESTART      = offsetof(NRF_NFCT_Type, EVENTS_RXFRAMESTART),      /**< The end of the first symbol of a received frame. */
    NRF_NFCT_EVENT_RXFRAMEEND        = offsetof(NRF_NFCT_Type, EVENTS_RXFRAMEEND),        /**< Received data was checked (CRC, parity) and transferred to RAM, and EasyDMA ended accessing the RX buffer. */
    NRF_NFCT_EVENT_ERROR             = offsetof(NRF_NFCT_Type, EVENTS_ERROR),             /**< NFC error reported. The ERRORSTATUS register contains details on the source of the error. */
    NRF_NFCT_EVENT_RXERROR           = offsetof(NRF_NFCT_Type, EVENTS_RXERROR),           /**< NFC RX frame error reported. The FRAMESTATUS.RX register contains details on the source of the error. */
    NRF_NFCT_EVENT_ENDRX             = offsetof(NRF_NFCT_Type, EVENTS_ENDRX),             /**< RX buffer (as defined by PACKETPTR and MAXLEN) in Data RAM full. */
    NRF_NFCT_EVENT_ENDTX             = offsetof(NRF_NFCT_Type, EVENTS_ENDTX),             /**< Transmission of data in RAM ended, and EasyDMA ended accessing the TX buffer. */
    NRF_NFCT_EVENT_AUTOCOLRESSTARTED = offsetof(NRF_NFCT_Type, EVENTS_AUTOCOLRESSTARTED), /**< Auto collision resolution process started. */
    NRF_NFCT_EVENT_COLLISION         = offsetof(NRF_NFCT_Type, EVENTS_COLLISION),         /**< NFC auto collision resolution error reported. */
    NRF_NFCT_EVENT_SELECTED          = offsetof(NRF_NFCT_Type, EVENTS_SELECTED),          /**< NFC auto collision resolution successfully completed. */
    NRF_NFCT_EVENT_STARTED           = offsetof(NRF_NFCT_Type, EVENTS_STARTED),           /**< EasyDMA is ready to receive or send frames. */
} nrf_nfct_event_t;

/** @brief NFCT shortcuts. */
typedef enum
{
    NRF_NFCT_SHORT_FIELDDETECTED_ACTIVATE_MASK  = NFCT_SHORTS_FIELDDETECTED_ACTIVATE_Msk,  /**< Shortcut between the FIELDDETECTED event and the ACTIVATE task. */
    NRF_NFCT_SHORT_FIELDLOST_SENSE_MASK         = NFCT_SHORTS_FIELDLOST_SENSE_Msk,         /**< Shortcut between the FIELDLOST event and the SENSE task. */
#if defined(NFCT_SHORTS_TXFRAMEEND_ENABLERXDATA_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_NFCT_SHORT_TXFRAMEEND_ENABLERXDATA_MASK = NFCT_SHORTS_TXFRAMEEND_ENABLERXDATA_Msk, /**< Shortcut between the TXFRAMEEND event and the ENABLERXDATA task. */
#endif // defined(NFCT_SHORTS_TXFRAMEEND_ENABLERXDATA_Msk) || defined(__NRFX_DOXYGEN__)
} nrf_nfct_short_mask_t;

/** @brief NFCT interrupts. */
typedef enum
{
    NRF_NFCT_INT_READY_MASK             = NFCT_INTEN_READY_Msk,             /**< Interrupt on READY event. */
    NRF_NFCT_INT_FIELDDETECTED_MASK     = NFCT_INTEN_FIELDDETECTED_Msk,     /**< Interrupt on FIELDDETECTED event. */
    NRF_NFCT_INT_FIELDLOST_MASK         = NFCT_INTEN_FIELDLOST_Msk,         /**< Interrupt on FIELDLOST event. */
    NRF_NFCT_INT_TXFRAMESTART_MASK      = NFCT_INTEN_TXFRAMESTART_Msk,      /**< Interrupt on TXFRAMESTART event. */
    NRF_NFCT_INT_TXFRAMEEND_MASK        = NFCT_INTEN_TXFRAMEEND_Msk,        /**< Interrupt on TXFRAMEEND event. */
    NRF_NFCT_INT_RXFRAMESTART_MASK      = NFCT_INTEN_RXFRAMESTART_Msk,      /**< Interrupt on RXFRAMESTART event. */
    NRF_NFCT_INT_RXFRAMEEND_MASK        = NFCT_INTEN_RXFRAMEEND_Msk,        /**< Interrupt on RXFRAMEEND event. */
    NRF_NFCT_INT_ERROR_MASK             = NFCT_INTEN_ERROR_Msk,             /**< Interrupt on ERROR event. */
    NRF_NFCT_INT_RXERROR_MASK           = NFCT_INTEN_RXERROR_Msk,           /**< Interrupt on RXERROR event. */
    NRF_NFCT_INT_ENDRX_MASK             = NFCT_INTEN_ENDRX_Msk,             /**< Interrupt on ENDRX event. */
    NRF_NFCT_INT_ENDTX_MASK             = NFCT_INTEN_ENDTX_Msk,             /**< Interrupt on ENDTX event. */
    NRF_NFCT_INT_AUTOCOLRESSTARTED_MASK = NFCT_INTEN_AUTOCOLRESSTARTED_Msk, /**< Interrupt on AUTOCOLRESSTARTED event. */
    NRF_NFCT_INT_COLLISION_MASK         = NFCT_INTEN_COLLISION_Msk,         /**< Interrupt on COLLISION event. */
    NRF_NFCT_INT_SELECTED_MASK          = NFCT_INTEN_SELECTED_Msk,          /**< Interrupt on SELECTED event. */
    NRF_NFCT_INT_STARTED_MASK           = NFCT_INTEN_STARTED_Msk,           /**< Interrupt on STARTED event. */
} nrf_nfct_int_mask_t;

/** @brief NFC error status bit masks. */
typedef enum
{
    NRF_NFCT_ERROR_FRAMEDELAYTIMEOUT_MASK = NFCT_ERRORSTATUS_FRAMEDELAYTIMEOUT_Msk, /**< Timeout of the Frame Delay Timer (no frame transmission started in the FDT window). */
#if defined(NFCT_ERRORSTATUS_NFCFIELDTOOSTRONG_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_NFCT_ERROR_NFCFIELDTOOSTRONG_MASK = NFCT_ERRORSTATUS_NFCFIELDTOOSTRONG_Msk, /**< Field level is too high at maximum load resistance. */
#endif // defined(NFCT_ERRORSTATUS_NFCFIELDTOOSTRONG_Msk) || defined(__NRFX_DOXYGEN__)
#if defined(NFCT_ERRORSTATUS_NFCFIELDTOOWEAK_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_NFCT_ERROR_NFCFIELDTOOWEAK_MASK = NFCT_ERRORSTATUS_NFCFIELDTOOWEAK_Msk,     /**< Field level is too low at minimum load resistance. */
#endif // defined(NFCT_ERRORSTATUS_NFCFIELDTOOWEAK_Msk) || defined(__NRFX_DOXYGEN__)
} nrf_nfct_error_status_t;

/** @brief NFC received frame status bit masks. */
typedef enum
{
    NRF_NFCT_RX_FRAME_STATUS_CRC_MASK     = NFCT_FRAMESTATUS_RX_CRCERROR_Msk,     /**< CRC status mask. */
    NRF_NFCT_RX_FRAME_STATUS_PARITY_MASK  = NFCT_FRAMESTATUS_RX_PARITYSTATUS_Msk, /**< Parity status mask. */
    NRF_NFCT_RX_FRAME_STATUS_OVERRUN_MASK = NFCT_FRAMESTATUS_RX_OVERRUN_Msk,      /**< Overrun status mask. */
} nrf_nfct_rx_frame_status_t;

#if defined(NFCT_NFCTAGSTATE_NFCTAGSTATE_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief NFC tag state. */
typedef enum
{
    NRF_NFCT_TAG_STATE_DISABLED    = NFCT_NFCTAGSTATE_NFCTAGSTATE_Disabled,   /**< Disabled or sensing NFC field. */
    NRF_NFCT_TAG_STATE_RAMP_UP     = NFCT_NFCTAGSTATE_NFCTAGSTATE_RampUp,     /**< Ramping up. */
    NRF_NFCT_TAG_STATE_IDLE        = NFCT_NFCTAGSTATE_NFCTAGSTATE_Idle,       /**< Idle. */
    NRF_NFCT_TAG_STATE_RECEIVE     = NFCT_NFCTAGSTATE_NFCTAGSTATE_Receive,    /**< Receiving data. */
    NRF_NFCT_TAG_STATE_FRAME_DELAY = NFCT_NFCTAGSTATE_NFCTAGSTATE_FrameDelay, /**< Counting Frame Delay Time since the last symbol of the last received frame. */
    NRF_NFCT_TAG_STATE_TRANSMIT    = NFCT_NFCTAGSTATE_NFCTAGSTATE_Transmit    /**< Transmitting data. */
} nrf_nfct_tag_state_t;
#endif // defined(NFCT_NFCTAGSTATE_NFCTAGSTATE_Msk) || defined(__NRFX_DOXYGEN__)

#if defined (NFCT_SLEEPSTATE_SLEEPSTATE_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief NFC tag sleep state.
 *
 * @details Shows the sleep state during automatic collision resolution
 *          according to the NFC Forum Activity Technical Specification v2.0.
 */
typedef enum
{
    NRF_NFCT_SLEEP_STATE_IDLE    = NFCT_SLEEPSTATE_SLEEPSTATE_Idle,  /**< 'IDLE' state. */
    NRF_NFCT_SLEEP_STATE_SLEEP_A = NFCT_SLEEPSTATE_SLEEPSTATE_SleepA /**< 'SLEEP_A' state. */
} nrf_nfct_sleep_state_t;
#endif // defined (NFCT_SLEEPSTATE_SLEEPSTATE_Msk) || defined(__NRFX_DOXYGEN__)

/** @brief NFC field state bit masks. */
typedef enum
{
    NRF_NFCT_FIELD_STATE_PRESENT_MASK = NFCT_FIELDPRESENT_FIELDPRESENT_Msk, /**< Field presence mask. */
    NRF_NFCT_FIELD_STATE_LOCK_MASK    = NFCT_FIELDPRESENT_LOCKDETECT_Msk    /**< Field lock mask. */
} nrf_nfct_field_state_t;

/** @brief NFC frame delay mode for data transmission. */
typedef enum
{
    NRF_NFCT_FRAME_DELAY_MODE_FREERUN    = NFCT_FRAMEDELAYMODE_FRAMEDELAYMODE_FreeRun,   /**< Frame transmission starts when @ref NRF_NFCT_TASK_STARTTX is set (delay timer is not used). */
    NRF_NFCT_FRAME_DELAY_MODE_WINDOW     = NFCT_FRAMEDELAYMODE_FRAMEDELAYMODE_Window,    /**< Frame transmission starts in a window between FRAMEDELAYMIN and FRAMEDELAYMAX. */
    NRF_NFCT_FRAME_DELAY_MODE_EXACTVAL   = NFCT_FRAMEDELAYMODE_FRAMEDELAYMODE_ExactVal,  /**< Frame transmission starts when the delay timer reaches FRAMEDELAYMAX. */
    NRF_NFCT_FRAME_DELAY_MODE_WINDOWGRID = NFCT_FRAMEDELAYMODE_FRAMEDELAYMODE_WindowGrid /**< Frame transmission starts in a bit grid between FRAMEDELAYMIN and FRAMEDELAYMAX. */
} nrf_nfct_frame_delay_mode_t;

/** @brief Bit masks for NFC transmission frame configuration. */
typedef enum
{
    NRF_NFCT_TX_FRAME_CONFIG_PARITY        = NFCT_TXD_FRAMECONFIG_PARITY_Msk,      /**< Indicates whether parity is added in the transmitted frames. */
    NRF_NFCT_TX_FRAME_CONFIG_DISCARD_START = NFCT_TXD_FRAMECONFIG_DISCARDMODE_Msk, /**< Indicates whether unused bits are discarded at the start or at the end of the transmitted frames. */
    NRF_NFCT_TX_FRAME_CONFIG_SOF           = NFCT_TXD_FRAMECONFIG_SOF_Msk,         /**< Indicates whether SoF symbol is added in the transmitted frames. */
    NRF_NFCT_TX_FRAME_CONFIG_CRC16         = NFCT_TXD_FRAMECONFIG_CRCMODETX_Msk    /**< Indicates whether CRC is added in the transmitted frames. */
} nrf_nfct_tx_frame_config_t;

/** @brief Bit masks for NFC reception frame configuration. */
typedef enum
{
    NRF_NFCT_RX_FRAME_CONFIG_PARITY = NFCT_RXD_FRAMECONFIG_PARITY_Msk,   /**< Indicates whether parity is expected in the received frames. */
    NRF_NFCT_RX_FRAME_CONFIG_SOF    = NFCT_RXD_FRAMECONFIG_SOF_Msk,      /**< Indicates whether SoF symbol is expected in the received frames. */
    NRF_NFCT_RX_FRAME_CONFIG_CRC16  = NFCT_RXD_FRAMECONFIG_CRCMODERX_Msk /**< Indicates whether CRC is expected and checked in the received frames. */
} nrf_nfct_rx_frame_config_t;

/**
 * @brief 'NFCI1 size' NFC field configuration for the SENS_RES frame according to the NFC Forum
 *        Digital Protocol Technical Specification.
 */
typedef enum
{
    NRF_NFCT_SENSRES_NFCID1_SIZE_SINGLE =
        NFCT_SENSRES_NFCIDSIZE_NFCID1Single << NFCT_SENSRES_NFCIDSIZE_Pos, /**< Single size NFCID1 (4 bytes). */
    NRF_NFCT_SENSRES_NFCID1_SIZE_DOUBLE =
        NFCT_SENSRES_NFCIDSIZE_NFCID1Double << NFCT_SENSRES_NFCIDSIZE_Pos, /**< Double size NFCID1 (7 bytes). */
    NRF_NFCT_SENSRES_NFCID1_SIZE_TRIPLE =
        NFCT_SENSRES_NFCIDSIZE_NFCID1Triple << NFCT_SENSRES_NFCIDSIZE_Pos, /**< Triple size NFCID1 (10 bytes). */
    NRF_NFCT_SENSRES_NFCID1_SIZE_DEFAULT =
        NFCT_SENSRES_NFCIDSIZE_Msk                                         /**< Default size. Use this option to leave NFCID1 size unchanged. */
} nrf_nfct_sensres_nfcid1_size_t;

/**
 * @brief 'Bit frame SDD' NFC field configuration for the SENS_RES frame according to the NFC
 *         Forum Digital Protocol Technical Specification.
 */
typedef enum
{
    NRF_NFCT_SENSRES_BIT_FRAME_SDD_00000 =
        NFCT_SENSRES_BITFRAMESDD_SDD00000 << NFCT_SENSRES_BITFRAMESDD_Pos, /**< SDD pattern 00000. */
    NRF_NFCT_SENSRES_BIT_FRAME_SDD_00001 =
        NFCT_SENSRES_BITFRAMESDD_SDD00001 << NFCT_SENSRES_BITFRAMESDD_Pos, /**< SDD pattern 00001. */
    NRF_NFCT_SENSRES_BIT_FRAME_SDD_00010 =
        NFCT_SENSRES_BITFRAMESDD_SDD00010 << NFCT_SENSRES_BITFRAMESDD_Pos, /**< SDD pattern 00010. */
    NRF_NFCT_SENSRES_BIT_FRAME_SDD_00100 =
        NFCT_SENSRES_BITFRAMESDD_SDD00100 << NFCT_SENSRES_BITFRAMESDD_Pos, /**< SDD pattern 00100. */
    NRF_NFCT_SENSRES_BIT_FRAME_SDD_01000 =
        NFCT_SENSRES_BITFRAMESDD_SDD01000 << NFCT_SENSRES_BITFRAMESDD_Pos, /**< SDD pattern 01000. */
    NRF_NFCT_SENSRES_BIT_FRAME_SDD_10000 =
        NFCT_SENSRES_BITFRAMESDD_SDD10000 << NFCT_SENSRES_BITFRAMESDD_Pos  /**< SDD pattern 10000. */
} nrf_nfct_sensres_bit_frame_sdd_t;

/**
 * @brief 'Platofrm Config' NFC field configuration for the SENS_RES frame according to the NFC
 *        Forum Digital Protocol Technical Specification.
 */
typedef enum
{
    /**< SENS_RES 'Platform Config' field (b4-b1) value for Type 1 Tag platform. */
    NRF_NFCT_SENSRES_PLATFORM_CONFIG_T1T   = 6 << NFCT_SENSRES_PLATFCONFIG_Pos,
    /**< SENS_RES 'Platform Config' field (b7-b6) value for any platform except Type 1 Tag platform. */
    NRF_NFCT_SENSRES_PLATFORM_CONFIG_OTHER = 0 << NFCT_SENSRES_PLATFCONFIG_Pos
} nrf_nfct_sensres_platform_config_t;

/** @brief Bit masks for SEL_RES NFC frame configuration. */
typedef enum
{
    NRF_NFCT_SELRES_CASCADE_MASK  = NFCT_SELRES_CASCADE_Msk,  /**< SEL_RES Cascade field bit mask. */
    NRF_NFCT_SELRES_PROTOCOL_MASK = NFCT_SELRES_PROTOCOL_Msk  /**< SEL_RES Protocol field bit mask. */
} nrf_nfct_selres_t;

/**
 * @brief Protocol NFC field (bits b7 and b6) configuration for the SEL_RES frame according to
 *        the NFC Forum Digital Protocol Technical Specification.
 */
typedef enum
{
    NRF_NFCT_SELRES_PROTOCOL_T2T         = 0,  /**< Type 2 Tag platform. */
    NRF_NFCT_SELRES_PROTOCOL_T4AT        = 1,  /**< Type 4A Tag platform. */
    NRF_NFCT_SELRES_PROTOCOL_NFCDEP      = 2,  /**< NFC-DEP Protocol. */
    NRF_NFCT_SELRES_PROTOCOL_NFCDEP_T4AT = 3,  /**< NFC-DEP Protocol and Type 4A Tag platform). */
} nrf_nfct_selres_protocol_t;

/**
 * @brief Function for activating a specific NFCT task.
 *
 * @param[in] task Task to be activated.
 */
__STATIC_INLINE void nrf_nfct_task_trigger(nrf_nfct_task_t task);

/**
 * @brief Function for returning the address of a specific NFCT task register.
 *
 * @param[in] task Task.
 *
 * @return Task address.
 */
__STATIC_INLINE uint32_t nrf_nfct_task_address_get(nrf_nfct_task_t task);

/**
 * @brief Function for clearing a specific event.
 *
 * @param[in] event Event.
 */
__STATIC_INLINE void nrf_nfct_event_clear(nrf_nfct_event_t event);

/**
 * @brief Function for retrieving the state of the NFCT event.
 *
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
__STATIC_INLINE bool nrf_nfct_event_check(nrf_nfct_event_t event);

/**
 * @brief Function for returning the address of a specific NFCT event register.
 *
 * @param[in] event Event.
 *
 * @return Address.
 */
__STATIC_INLINE uint32_t nrf_nfct_event_address_get(nrf_nfct_event_t event);

/**
 * @brief Function for enabling selected shortcuts.
 *
 * @param[in] short_mask Mask of shortcuts.
 */
__STATIC_INLINE void nrf_nfct_shorts_enable(uint32_t short_mask);

/**
 * @brief Function for disabling selected shortcuts.
 *
 * @param[in] short_mask Mask of shortcuts.
 */
__STATIC_INLINE void nrf_nfct_shorts_disable(uint32_t short_mask);

/**
 * @brief Function for retrieving the enabled shortcuts.
 *
 * @return Flags of the currently enabled shortcuts.
 */
__STATIC_INLINE uint32_t nrf_nfct_shorts_get(void);

/**
 * @brief Function for setting shortcuts.
 *
 * @param[in] short_mask Shortcut mask.
 */
__STATIC_INLINE void nrf_nfct_shorts_set(uint32_t short_mask);

/**
 * @brief Function for enabling the selected interrupts.
 *
 * @param[in] int_mask Interrupt mask.
 */
__STATIC_INLINE void nrf_nfct_int_enable(uint32_t int_mask);

/**
 * @brief Function for retrieving the state of the selected interrupts.
 *
 * @param[in] int_mask Interrupt mask.
 *
 * @retval true  Any of the selected interrupts is enabled.
 * @retval false None of the selected interrupts is enabled.
 */
__STATIC_INLINE bool nrf_nfct_int_enable_check(uint32_t int_mask);

/**
 * @brief Function for retrieving the information about enabled interrupts.
 *
 * @return The flags of the enabled interrupts.
 */
__STATIC_INLINE uint32_t nrf_nfct_int_enable_get(void);

/**
 * @brief Function for disabling the selected interrupts.
 *
 * @param[in] int_mask Interrupt mask.
 */
__STATIC_INLINE void nrf_nfct_int_disable(uint32_t int_mask);

/**
 * @brief Function for getting the NFCT error status.
 *
 * @return The NFCT error status flags, defined in @ref nrf_nfct_error_status_t.
 */
__STATIC_INLINE uint32_t nrf_nfct_error_status_get(void);

/**
 * @brief Function for clearing the NFCT error status.
 *
 * @param[in] error_flag Error flags to be cleared, defined in @ref nrf_nfct_error_status_t.
 */
__STATIC_INLINE void nrf_nfct_error_status_clear(uint32_t error_flag);

/**
 * @brief Function for getting the NFC frame reception status.
 *
 * @return The flags of the NFC frame reception status, defined in @ref nrf_nfct_rx_frame_status_t.
 */
__STATIC_INLINE uint32_t nrf_nfct_rx_frame_status_get(void);

/**
 * @brief Function for clearing the NFC frame reception status.
 *
 * @param[in] framestatus_flags Status flags to be cleared, defined in
 *                              @ref nrf_nfct_rx_frame_status_t.
 */
__STATIC_INLINE void nrf_nfct_rx_frame_status_clear(uint32_t framestatus_flags);

#if defined(NFCT_NFCTAGSTATE_NFCTAGSTATE_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for getting the NFC tag state.
 *
 * @retval NRF_NFCT_TAG_STATE_DISABLED    NFC tag is disabled.
 * @retval NRF_NFCT_TAG_STATE_RAMP_UP     NFC tag is ramping up.
 * @retval NRF_NFCT_TAG_STATE_IDLE        NFC tag is activated and idle.
 * @retval NRF_NFCT_TAG_STATE_RECEIVE     NFC tag is receiving data.
 * @retval NRF_NFCT_TAG_STATE_FRAME_DELAY Frame Delay Timer of the NFC tag is counting ticks
 *                                        since the last symbol of the last received frame.
 * @retval NRF_NFCT_TAG_STATE_TRANSMIT    NFC tag is transmitting data.
 */
__STATIC_INLINE nrf_nfct_tag_state_t nrf_nfct_tag_state_get(void);
#endif // defined(NFCT_NFCTAGSTATE_NFCTAGSTATE_Msk) || defined(__NRFX_DOXYGEN__)

#if defined (NFCT_SLEEPSTATE_SLEEPSTATE_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for getting the NFC tag sleep state during the automatic collision resolution.
 *
 * @details The returned value is the last state before the autimatic collision resolution started.
 *
 * @retval NRF_NFCT_SLEEP_STATE_IDLE    NFC tag was in IDLE state before the automatic
 *                                      collision resolution started.
 * @retval NRF_NFCT_SLEEP_STATE_SLEEP_A NFC tag was in SLEEP_A state before the automatic
 *                                      collision resolution started.
 */
__STATIC_INLINE nrf_nfct_sleep_state_t nrf_nfct_sleep_state_get(void);
#endif // defined (NFCT_SLEEPSTATE_SLEEPSTATE_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for getting the status of the external NFC field detection.
 *
 * @return The NFC field detection status. Status bits can be checked by using
 *         @ref nrf_nfct_field_state_t.
 */
__STATIC_INLINE uint8_t nrf_nfct_field_status_get(void);

/**
 * @brief Function for getting the minimum Frame Delay Time value.
 *
 * @details This is the minimum value for Frame Delay Timer. It controls the shortest time between
 *          the last symbol of the last received frame and the start of the transmission of a new
 *          TX frame.
 *
 * @return The minimum Frame Delay Time value in 13.56-MHz clock ticks.
 */
__STATIC_INLINE uint16_t nrf_nfct_frame_delay_min_get(void);

/**
 * @brief Function for setting the minimum Frame Delay Time value.
 *
 * @details This is the minimum value for Frame Delay Timer. It controls the shortest time between
 *          the last symbol of the last received frame and the start of the transmission of a new
 *          TX frame.
 *
 * @param[in] frame_delay_min Minimum Frame Delay Time value in 13.56-MHz clock ticks.
 */
__STATIC_INLINE void nrf_nfct_frame_delay_min_set(uint16_t frame_delay_min);

/**
 * @brief Function for getting the maximum Frame Delay Time value.
 *
 * @details This is the maximum value for Frame Delay Timer. It controls the longest time between
 *          the last symbol of the last received frame and the start of the transmission of a new
 *          TX frame. If no transmission starts before the Frame Delay Timer timeout,
 *          @ref NRF_NFCT_ERROR_FRAMEDELAYTIMEOUT_MASK is set.
 *
 * @return The maximum Frame Delay Time value in 13.56-MHz clock ticks.
 */
__STATIC_INLINE uint32_t nrf_nfct_frame_delay_max_get(void);

/**
 * @brief Function for setting the maximum Frame Delay Time value.
 *
 * @details This is the maximum value for Frame Delay Timer. It controls the longest time between
 *          the last symbol of the last received frame and the start of the transmission of a new
 *          TX frame. If no transmission starts before the Frame Delay Timer timeout,
 *          @ref NRF_NFCT_ERROR_FRAMEDELAYTIMEOUT_MASK is set.
 *
 * @param[in] frame_delay_max Maximum Frame Delay Time value in 13.56-MHz clock ticks.
 */
__STATIC_INLINE void nrf_nfct_frame_delay_max_set(uint32_t frame_delay_max);

/**
 * @brief Function for getting the Frame Delay Mode configuration.
 *
 * @return The configured Frame Delay Mode.
 */
__STATIC_INLINE nrf_nfct_frame_delay_mode_t nrf_nfct_frame_delay_mode_get(void);

/**
 * @brief Function for setting the NFC Frame Delay Mode configuration.
 *
 * @param[in] frame_delay_mode Frame Delay Mode configuration.
 */
__STATIC_INLINE void nrf_nfct_frame_delay_mode_set(nrf_nfct_frame_delay_mode_t frame_delay_mode);

/**
 * @brief Function for getting the pointer to the NFCT RX/TX buffer.
 *
 * @return The configured pointer to the receive or transmit buffer.
 */
__STATIC_INLINE uint8_t * nrf_nfct_rxtx_buffer_get(void);

/**
 * @brief Function for setting the the NFCT RX/TX buffer (address and maximum length).
 *
 * @note Buffer for the NFC RX/TX data is used by EasyDMA and must be located in RAM.
 *
 * @param[in] p_rxtx_buf   Pointer to the receive or transmit buffer.
 * @param[in] max_txrx_len Maximum receive or transmit length in bytes (size of the RAM
 *                         buffer for EasyDMA).
 */
__STATIC_INLINE void nrf_nfct_rxtx_buffer_set(uint8_t * p_rxtx_buf,
                                              uint16_t  max_txrx_len);

/**
 * @brief Function for getting the NFCT RX/TX maximum buffer length.
 *
 * @return The configured maximum receive or transmit length in bytes (size of the RX/TX
 *         buffer for EasyDMA).
 */
__STATIC_INLINE uint16_t nrf_nfct_max_rxtx_length_get(void);

/**
 * @brief Function for getting the flags for NFC frame transmission configuration.
 *
 * @return The flags of the NFCT frame transmission configuration, defined in
 *         @ref nrf_nfct_tx_frame_config_t.
 */
__STATIC_INLINE uint8_t nrf_nfct_tx_frame_config_get(void);

/**
 * @brief Function for setting up the flags of the NFC frame transmission configuration.
 *
 * @param[in] flags Flags for NFCT TX configuration. Use @ref nrf_nfct_tx_frame_config_t for
 *                  setting.
 */
__STATIC_INLINE void nrf_nfct_tx_frame_config_set(uint8_t flags);

/**
 * @brief Function for getting the length of the configured transmission frame.
 *
 * @note NFC frames do not have to consist of full bytes only, therefore data amount
 *       for transmission is configured in number of bits.
 *
 * @return Number of bits to be sent excluding CRC, parity, SoF, and EoF.
 */
__STATIC_INLINE uint16_t nrf_nfct_tx_bits_get(void);

/**
 * @brief Function for setting up the NFC frame transmission.
 *
 * @details Set the number of TX bits excluding CRC, parity, SoF, and EoF.
 *
 * @note Source of data for transmission is set by using @ref nrf_nfct_rxtx_buffer_set.
 * @note NFC frames do not have to consist of full bytes only, therefore data amount
 *       for transmission is configured in number of bits.
 *
 * @param[in] tx_bits Overall number of bits to be sent.
 */
__STATIC_INLINE void nrf_nfct_tx_bits_set(uint16_t tx_bits);

/**
 * @brief Function for getting the flags of the NFC frame reception configuration.
 *
 * @return The flags for NFCT frame reception configuration, defined in
 *         @ref nrf_nfct_rx_frame_config_t.
 */
__STATIC_INLINE uint8_t nrf_nfct_rx_frame_config_get(void);

/**
 * @brief Function for setting up the NFC frame reception.
 *
 * @note Destination for the received data is set using @ref nrf_nfct_rxtx_buffer_set.
 *
 * @param[in] flags NFCT RX configuration flags. Use @ref nrf_nfct_rx_frame_config_t for setting
 *                  the desired configuration.
 */
__STATIC_INLINE void nrf_nfct_rx_frame_config_set(uint8_t flags);

/**
 * @brief Function for getting the number of bits received from the NFC poller.
 *
 * @param[in] crc_excluded Flag for excluding CRC size from calculation.
 *
 * @return Number of received bits including or excluding CRC, and excluding parity
 *         and SoF/EoF framing.
 */
__STATIC_INLINE uint16_t nrf_nfct_rx_bits_get(bool crc_excluded);

/**
 * @brief Function for getting the NFCID1 (NFC tag identifier).
 *
 * @note This function always returns the full configuration of the NFCID1 setting (10 bytes),
 *       regardless of the NFCID1 size. The NFCID1 size can be configured using
 *       @ref nrf_nfct_sensres_nfcid1_size_set or @ref nrf_nfct_nfcid1_set.
 *
 * @param[out] p_nfcid1_buf Pointer to a buffer for the NDFCID1 parameter.
 *                          The NFCID1 values are in little endian order,
 *                          that is: |NFCID1_3RD_LAST|NFCID1_2ND_LAST|NFCID1_LAST|.
 *
 * @return Configured NFCID1 length
 */
__STATIC_INLINE nrf_nfct_sensres_nfcid1_size_t nrf_nfct_nfcid1_get(uint8_t * p_nfcid1_buf);

/**
 * @brief Function for setting the NFCID1 (NFC tag identifier).
 *
 * @note This function also configures the NFCIDSIZE field in the SENSRES
 *       register of the NRF_NFCT peripheral.
 *
 * @param[in] p_nfcid1_buf Pointer to the buffer with NDFCID1 bytes.
 * @param[in] nfcid1_size  Size of the NFCID1 in bytes.
 */
__STATIC_INLINE void nrf_nfct_nfcid1_set(uint8_t const *                p_nfcid1_buf,
                                         nrf_nfct_sensres_nfcid1_size_t nfcid1_size);

#if defined (NFCT_AUTOCOLRESCONFIG_MODE_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for getting the setting for the automatic collision resolution.
 *
 * @details The automatic collision resolution mechanism as defined in ISO 14443-3 and NFC Forum
 *          Digital Protocol Technical Specification 2.0, section 6.
 *
 * @retval true  If automatic collision resolution is enabled.
 * @retval false If automatic collision resolution is disabled.
 */
__STATIC_INLINE bool nrf_nfct_autocolres_is_enabled(void);

/**
 * @brief Function for enabling the automatic collision resolution.
 *
 * @details The automatic collision resolution mechanism as defined in ISO 14443-3 and NFC Forum
 *          Digital Protocol Technical Specification 2.0, section 6.
 */
__STATIC_INLINE void nrf_nfct_autocolres_enable(void);

/**
 * @brief Function for disabling the automatic collision resolution.
 *
 * @details The automatic collision resolution mechanism as defined in ISO 14443-3 and NFC Forum
 *          Digital Protocol Technical Specification 2.0, section 6.
 */
__STATIC_INLINE void nrf_nfct_autocolres_disable(void);
#endif // defined (NFCT_AUTOCOLRESCONFIG_MODE_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for getting the NFCID1 size from the SENS_RES frame configuration.
 *
 * @details The SENS_RES frame is handled automatically by the NFCT hardware.
 *
 * @return NFCID1 (tag identifier) size.
 */
__STATIC_INLINE nrf_nfct_sensres_nfcid1_size_t nrf_nfct_sensres_nfcid1_size_get(void);

/**
 * @brief Function for setting the NFCID1 (tag identifier) size.field in the SENS_RES frame
 *        configuration.
 *
 * @note The SENS_RES frame is handled automatically by the NFCT hardware.
 *
 * @param[in] nfcid1_size NFCID1 (tag identifier) size.
 *
 * @sa nrf_nfct_nfcid1_set()
 */
__STATIC_INLINE void nrf_nfct_sensres_nfcid1_size_set(nrf_nfct_sensres_nfcid1_size_t nfcid1_size);

/**
 * @brief Function for getting the Bit Frame SDD field from the SENS_RES frame configuration.
 *
 * @details The SENS_RES frame is handled automatically by the NFCT hardware.
 *
 * @return The Bit Frame SDD field configuration.
 */
__STATIC_INLINE nrf_nfct_sensres_bit_frame_sdd_t nrf_nfct_sensres_bit_frame_sdd_get(void);

/**
 * @brief Function for setting the Bit Frame SDD field in the SENS_RES frame configuration.
 *
 * @note The SENS_RES frame is handled automatically by the NFCT hardware.
 *
 * @param[in] bit_frame_sdd The Bit Frame SDD field configuration.
 */
__STATIC_INLINE void nrf_nfct_sensres_bit_frame_sdd_set(nrf_nfct_sensres_bit_frame_sdd_t bit_frame_sdd);

/**
 * @brief Function for getting the Platform Config field from the SENS_RES frame configuration.
 *
 * @details The SENS_RES frame is handled automatically by the NFCT hardware.
 *
 * @return The Platform Config field configuration.
 */
__STATIC_INLINE nrf_nfct_sensres_platform_config_t nrf_nfct_sensres_platform_config_get(void);

/**
 * @brief Function for setting the Platform Config field in the SENS_RES frame configuration.
 *
 * @note The SENS_RES frame is handled automatically by the NFCT hardware.
 *
 * @param[in] platform_config The Platform Config field configuration.
 */
__STATIC_INLINE void nrf_nfct_sensres_platform_config_set(nrf_nfct_sensres_platform_config_t platform_config);

/**
 * @brief Function for checking the CASCADE bit of the SEL_RES frame.
 *
 * @details The CASCADE bit in the SEL_RES register is handled automatically by the NFCT hardware
 *          and indicates the status of the NFCID1 read operation to the NFC poller according to
 *          the NFC Forum Digital Protocol Speficiation 2.0, section 6.8.2.
 *
 * @retval true  If NFCID1 read procedure is not complete.
 * @retval false If NFCID1 read procedure is complete.
 */
__STATIC_INLINE bool nrf_nfct_selsres_cascade_check(void);

/**
 * @brief Function for getting the Protocol field in the SEL_RES frame.
 *
 * @details The SEL_RES frame is handled automatically by the NFCT hardware.
 *
 * @return Value of the Protocol field in the SEL_RES frame.
 */
__STATIC_INLINE nrf_nfct_selres_protocol_t nrf_nfct_selsres_protocol_get(void);

/**
 * @brief Function for setting the Protocol field in the SEL_RES frame configuration.
 *
 * @details The SEL_RES frame is handled automatically by the NFCT hardware.
 *
 * @param[in] sel_res_protocol Value of the Protocol field in the SEL_RES frame.
 */
__STATIC_INLINE void nrf_nfct_selres_protocol_set(nrf_nfct_selres_protocol_t sel_res_protocol);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION
__STATIC_INLINE void nrf_nfct_task_trigger(nrf_nfct_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_NFCT + (uint32_t)task)) = 1UL;
}

__STATIC_INLINE uint32_t nrf_nfct_task_address_get(nrf_nfct_task_t task)
{
    return (uint32_t)((uint8_t *)NRF_NFCT + (uint32_t)task);
}

__STATIC_INLINE void nrf_nfct_event_clear(nrf_nfct_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_NFCT + (uint32_t)event)) = 0UL;
    __DSB();
}

__STATIC_INLINE bool nrf_nfct_event_check(nrf_nfct_event_t event)
{
    return (bool)*(volatile const uint32_t *)((uint8_t *)NRF_NFCT + (uint32_t)event);
}

__STATIC_INLINE uint32_t nrf_nfct_event_address_get(nrf_nfct_event_t event)
{
    return (uint32_t)((uint8_t *)NRF_NFCT + (uint32_t)event);
}

__STATIC_INLINE void nrf_nfct_shorts_enable(uint32_t short_mask)
{
    NRF_NFCT->SHORTS |= short_mask;
}

__STATIC_INLINE void nrf_nfct_shorts_disable(uint32_t short_mask)
{
    NRF_NFCT->SHORTS &= ~short_mask;
}

__STATIC_INLINE uint32_t nrf_nfct_shorts_get(void)
{
    return NRF_NFCT->SHORTS;
}

__STATIC_INLINE void nrf_nfct_shorts_set(uint32_t short_mask)
{
    NRF_NFCT->SHORTS = short_mask;
}

__STATIC_INLINE void nrf_nfct_int_enable(uint32_t int_mask)
{
    NRF_NFCT->INTENSET = int_mask;
}

__STATIC_INLINE bool nrf_nfct_int_enable_check(uint32_t int_mask)
{
    return (bool)(NRF_NFCT->INTENSET & int_mask);
}

__STATIC_INLINE uint32_t nrf_nfct_int_enable_get(void)
{
    return NRF_NFCT->INTENSET;
}

__STATIC_INLINE void nrf_nfct_int_disable(uint32_t int_mask)
{
    NRF_NFCT->INTENCLR = int_mask;
}

__STATIC_INLINE uint32_t nrf_nfct_error_status_get(void)
{
    return NRF_NFCT->ERRORSTATUS;
}

__STATIC_INLINE void nrf_nfct_error_status_clear(uint32_t error_flags)
{
    NRF_NFCT->ERRORSTATUS = error_flags;
}

__STATIC_INLINE uint32_t nrf_nfct_rx_frame_status_get(void)
{
    return NRF_NFCT->FRAMESTATUS.RX;
}

__STATIC_INLINE void nrf_nfct_rx_frame_status_clear(uint32_t framestatus_flags)
{
    NRF_NFCT->FRAMESTATUS.RX = framestatus_flags;
}

#if defined(NFCT_NFCTAGSTATE_NFCTAGSTATE_Msk) || defined(__NRFX_DOXYGEN__)
__STATIC_INLINE nrf_nfct_tag_state_t nrf_nfct_tag_state_get(void)
{
    return (nrf_nfct_tag_state_t)((NRF_NFCT->NFCTAGSTATE & NFCT_NFCTAGSTATE_NFCTAGSTATE_Msk) >>
                                  NFCT_NFCTAGSTATE_NFCTAGSTATE_Pos);
}
#endif // defined(NFCT_NFCTAGSTATE_NFCTAGSTATE_Msk) || defined(__NRFX_DOXYGEN__)

#if defined (NFCT_SLEEPSTATE_SLEEPSTATE_Msk) || defined(__NRFX_DOXYGEN__)
__STATIC_INLINE nrf_nfct_sleep_state_t nrf_nfct_sleep_state_get(void)
{
    return (nrf_nfct_sleep_state_t)((NRF_NFCT->SLEEPSTATE & NFCT_SLEEPSTATE_SLEEPSTATE_Msk) >>
                                    NFCT_SLEEPSTATE_SLEEPSTATE_Pos);
}
#endif // defined (NFCT_SLEEPSTATE_SLEEPSTATE_Msk) || defined(__NRFX_DOXYGEN__)

__STATIC_INLINE uint8_t nrf_nfct_field_status_get(void)
{
    return (uint8_t)(NRF_NFCT->FIELDPRESENT);
}

__STATIC_INLINE uint16_t nrf_nfct_frame_delay_min_get(void)
{
    return (uint16_t)((NRF_NFCT->FRAMEDELAYMIN & NFCT_FRAMEDELAYMIN_FRAMEDELAYMIN_Msk) >>
                      NFCT_FRAMEDELAYMIN_FRAMEDELAYMIN_Pos);
}

__STATIC_INLINE void nrf_nfct_frame_delay_min_set(uint16_t frame_delay_min)
{
    NRF_NFCT->FRAMEDELAYMIN =
        ((uint32_t)frame_delay_min << NFCT_FRAMEDELAYMIN_FRAMEDELAYMIN_Pos) &
        NFCT_FRAMEDELAYMIN_FRAMEDELAYMIN_Msk;
}

__STATIC_INLINE uint32_t nrf_nfct_frame_delay_max_get(void)
{
    return (NRF_NFCT->FRAMEDELAYMAX & NFCT_FRAMEDELAYMAX_FRAMEDELAYMAX_Msk) >>
           NFCT_FRAMEDELAYMAX_FRAMEDELAYMAX_Pos;
}

__STATIC_INLINE void nrf_nfct_frame_delay_max_set(uint32_t frame_delay_max)
{
    NRF_NFCT->FRAMEDELAYMAX =
        ((uint32_t)frame_delay_max << NFCT_FRAMEDELAYMAX_FRAMEDELAYMAX_Pos) &
        NFCT_FRAMEDELAYMAX_FRAMEDELAYMAX_Msk;
}

__STATIC_INLINE nrf_nfct_frame_delay_mode_t nrf_nfct_frame_delay_mode_get(void)
{
    return (nrf_nfct_frame_delay_mode_t)(NRF_NFCT->FRAMEDELAYMODE &
                                         NFCT_FRAMEDELAYMODE_FRAMEDELAYMODE_Msk);
}

__STATIC_INLINE void nrf_nfct_frame_delay_mode_set(nrf_nfct_frame_delay_mode_t frame_delay_mode)
{
    NRF_NFCT->FRAMEDELAYMODE = (uint32_t)frame_delay_mode;
}

__STATIC_INLINE uint8_t * nrf_nfct_rxtx_buffer_get(void)
{
    return (uint8_t *)(NRF_NFCT->PACKETPTR);
}

__STATIC_INLINE void nrf_nfct_rxtx_buffer_set(uint8_t * p_rxtx_buf,
                                              uint16_t  max_txrx_len)
{
    NRF_NFCT->PACKETPTR = (uint32_t)p_rxtx_buf;
    NRF_NFCT->MAXLEN    = ((uint32_t)max_txrx_len << NFCT_MAXLEN_MAXLEN_Pos) &
                                                     NFCT_MAXLEN_MAXLEN_Msk;
}

__STATIC_INLINE uint16_t nrf_nfct_max_rxtx_length_get(void)
{
    return (uint16_t)((NRF_NFCT->MAXLEN & NFCT_MAXLEN_MAXLEN_Msk) >> NFCT_MAXLEN_MAXLEN_Pos);
}

__STATIC_INLINE uint8_t nrf_nfct_tx_frame_config_get(void)
{
    return (uint8_t)(NRF_NFCT->TXD.FRAMECONFIG);
}

__STATIC_INLINE void nrf_nfct_tx_frame_config_set(uint8_t flags)
{
    NRF_NFCT->TXD.FRAMECONFIG = flags;
}

__STATIC_INLINE uint16_t nrf_nfct_tx_bits_get(void)
{
    return (uint16_t)(NRF_NFCT->TXD.AMOUNT & (NFCT_TXD_AMOUNT_TXDATABITS_Msk |
                                              NFCT_TXD_AMOUNT_TXDATABYTES_Msk));
}

__STATIC_INLINE void nrf_nfct_tx_bits_set(uint16_t tx_bits)
{
    NRF_NFCT->TXD.AMOUNT = (tx_bits & (NFCT_TXD_AMOUNT_TXDATABITS_Msk | NFCT_TXD_AMOUNT_TXDATABYTES_Msk));
}

__STATIC_INLINE uint8_t nrf_nfct_rx_frame_config_get(void)
{
    return (uint8_t)(NRF_NFCT->RXD.FRAMECONFIG);
}

__STATIC_INLINE void nrf_nfct_rx_frame_config_set(uint8_t flags)
{
    NRF_NFCT->RXD.FRAMECONFIG = flags;
}

__STATIC_INLINE uint16_t nrf_nfct_rx_bits_get(bool crc_excluded)
{
    uint16_t rx_bits = NRF_NFCT->RXD.AMOUNT & (NFCT_RXD_AMOUNT_RXDATABITS_Msk |
                                               NFCT_RXD_AMOUNT_RXDATABYTES_Msk);
    return rx_bits - (crc_excluded ? (8u * NRF_NFCT_CRC_SIZE) : 0);
}

__STATIC_INLINE nrf_nfct_sensres_nfcid1_size_t nrf_nfct_nfcid1_get(uint8_t * p_nfcid1_buf)
{
    uint32_t nfcid1_last = NRF_NFCT->NFCID1_LAST;
    nrf_nfct_sensres_nfcid1_size_t size =
        (nrf_nfct_sensres_nfcid1_size_t)(NRF_NFCT->SENSRES & NFCT_SENSRES_NFCIDSIZE_Msk);

    if (size != NRF_NFCT_SENSRES_NFCID1_SIZE_SINGLE)
    {
        uint32_t nfcid1_2nd_last = NRF_NFCT->NFCID1_2ND_LAST;

        if (size == NRF_NFCT_SENSRES_NFCID1_SIZE_TRIPLE)
        {
            uint32_t nfcid1_3rd_last = NRF_NFCT->NFCID1_3RD_LAST;

            *p_nfcid1_buf++ = (uint8_t)(nfcid1_3rd_last >> 16UL);
            *p_nfcid1_buf++ = (uint8_t)(nfcid1_3rd_last >> 8UL);
            *p_nfcid1_buf++ = (uint8_t)(nfcid1_3rd_last >> 0UL);
        }

        *p_nfcid1_buf++ = (uint8_t)(nfcid1_2nd_last >> 16UL);
        *p_nfcid1_buf++ = (uint8_t)(nfcid1_2nd_last >> 8UL);
        *p_nfcid1_buf++ = (uint8_t)(nfcid1_2nd_last >> 0UL);
    }

    *p_nfcid1_buf++ = (uint8_t)(nfcid1_last >> 24UL);
    *p_nfcid1_buf++ = (uint8_t)(nfcid1_last >> 16UL);
    *p_nfcid1_buf++ = (uint8_t)(nfcid1_last >> 8UL);
    *p_nfcid1_buf++ = (uint8_t)(nfcid1_last >> 0UL);

    return size;
}

__STATIC_INLINE void nrf_nfct_nfcid1_set(uint8_t const *                p_nfcid1_buf,
                                         nrf_nfct_sensres_nfcid1_size_t nfcid1_size)
{
    nrf_nfct_sensres_nfcid1_size_t size = (nfcid1_size == NRF_NFCT_SENSRES_NFCID1_SIZE_DEFAULT) ?
        NRF_NFCT_SENSRES_NFCID1_SIZE_DOUBLE : nfcid1_size;

    if (size != NRF_NFCT_SENSRES_NFCID1_SIZE_SINGLE)
    {
        if (size == NRF_NFCT_SENSRES_NFCID1_SIZE_TRIPLE)
        {
            NRF_NFCT->NFCID1_3RD_LAST = ((uint32_t)p_nfcid1_buf[0] << 16UL) |
                                        ((uint32_t)p_nfcid1_buf[1] << 8UL)  |
                                        ((uint32_t)p_nfcid1_buf[2] << 0UL);
            p_nfcid1_buf += 3UL;
        }
        NRF_NFCT->NFCID1_2ND_LAST = ((uint32_t)p_nfcid1_buf[0] << 16UL) |
                                    ((uint32_t)p_nfcid1_buf[1] << 8UL)  |
                                    ((uint32_t)p_nfcid1_buf[2] << 0UL);
        p_nfcid1_buf += 3UL;
    }

    NRF_NFCT->NFCID1_LAST = ((uint32_t)p_nfcid1_buf[0] << 24UL) |
                            ((uint32_t)p_nfcid1_buf[1] << 16UL) |
                            ((uint32_t)p_nfcid1_buf[2] << 8UL)  |
                            ((uint32_t)p_nfcid1_buf[3] << 0UL);

    NRF_NFCT->SENSRES = ((NRF_NFCT->SENSRES & ~NFCT_SENSRES_NFCIDSIZE_Msk) |
                         (uint32_t)size);
}

#if defined (NFCT_AUTOCOLRESCONFIG_MODE_Msk) || defined(__NRFX_DOXYGEN__)
__STATIC_INLINE bool nrf_nfct_autocolres_is_enabled(void)
{
    return (NRF_NFCT->AUTOCOLRESCONFIG & NFCT_AUTOCOLRESCONFIG_MODE_Msk) ==
           (NFCT_AUTOCOLRESCONFIG_MODE_Enabled << NFCT_AUTOCOLRESCONFIG_MODE_Pos);
}

__STATIC_INLINE void nrf_nfct_autocolres_enable(void)
{
    NRF_NFCT->AUTOCOLRESCONFIG =
        (NRF_NFCT->AUTOCOLRESCONFIG & ~NFCT_AUTOCOLRESCONFIG_MODE_Msk) |
        (NFCT_AUTOCOLRESCONFIG_MODE_Enabled << NFCT_AUTOCOLRESCONFIG_MODE_Pos);
}

__STATIC_INLINE void nrf_nfct_autocolres_disable(void)
{
    NRF_NFCT->AUTOCOLRESCONFIG =
        (NRF_NFCT->AUTOCOLRESCONFIG & ~NFCT_AUTOCOLRESCONFIG_MODE_Msk) |
        (NFCT_AUTOCOLRESCONFIG_MODE_Disabled << NFCT_AUTOCOLRESCONFIG_MODE_Pos);
}
#endif // defined (NFCT_AUTOCOLRESCONFIG_MODE_Msk) || defined(__NRFX_DOXYGEN__)

__STATIC_INLINE nrf_nfct_sensres_nfcid1_size_t nrf_nfct_sensres_nfcid1_size_get(void)
{
    return (nrf_nfct_sensres_nfcid1_size_t)(NRF_NFCT->SENSRES & NFCT_SENSRES_NFCIDSIZE_Msk);
}

__STATIC_INLINE void nrf_nfct_sensres_nfcid1_size_set(nrf_nfct_sensres_nfcid1_size_t nfcid1_size)
{
    NRF_NFCT->SENSRES = ((NRF_NFCT->SENSRES & ~(NFCT_SENSRES_NFCIDSIZE_Msk)) | (uint32_t)nfcid1_size);
}

__STATIC_INLINE nrf_nfct_sensres_bit_frame_sdd_t nrf_nfct_sensres_bit_frame_sdd_get(void)
{
    return (nrf_nfct_sensres_bit_frame_sdd_t)(NRF_NFCT->SENSRES & NFCT_SENSRES_BITFRAMESDD_Msk);
}

__STATIC_INLINE void nrf_nfct_sensres_bit_frame_sdd_set(nrf_nfct_sensres_bit_frame_sdd_t bit_frame_sdd)
{
    NRF_NFCT->SENSRES = ((NRF_NFCT->SENSRES & ~(NFCT_SENSRES_BITFRAMESDD_Msk)) | (uint32_t)bit_frame_sdd);
}

__STATIC_INLINE nrf_nfct_sensres_platform_config_t nrf_nfct_sensres_platform_config_get(void)
{
    return (nrf_nfct_sensres_platform_config_t)(NRF_NFCT->SENSRES & NFCT_SENSRES_PLATFCONFIG_Msk);
}

__STATIC_INLINE void nrf_nfct_sensres_platform_config_set(nrf_nfct_sensres_platform_config_t platform_config)
{
    NRF_NFCT->SENSRES = ((NRF_NFCT->SENSRES & ~(NFCT_SENSRES_PLATFCONFIG_Msk)) | (uint32_t)platform_config);
}

__STATIC_INLINE bool nrf_nfct_selsres_cascade_check(void)
{
    return (bool)(NRF_NFCT->SELRES & NFCT_SELRES_CASCADE_Msk);
}

__STATIC_INLINE nrf_nfct_selres_protocol_t nrf_nfct_selsres_protocol_get(void)
{
    return (nrf_nfct_selres_protocol_t)((NRF_NFCT->SELRES & NFCT_SELRES_PROTOCOL_Msk) >>
                                        NFCT_SELRES_PROTOCOL_Pos);
}

__STATIC_INLINE void nrf_nfct_selres_protocol_set(nrf_nfct_selres_protocol_t sel_res_protocol)
{
    NRF_NFCT->SELRES = (NRF_NFCT->SELRES & ~NFCT_SELRES_PROTOCOL_Msk) |
                       ((uint32_t)sel_res_protocol << NFCT_SELRES_PROTOCOL_Pos);
}
#endif /* SUPPRESS_INLINE_IMPLEMENTATION */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRF_NFCT_H__ */
