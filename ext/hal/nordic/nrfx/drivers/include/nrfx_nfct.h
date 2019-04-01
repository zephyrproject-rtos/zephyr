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

#ifndef NRFX_NFCT_H__
#define NRFX_NFCT_H__

#include <nrfx.h>
#include <hal/nrf_nfct.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_nfct NFCT driver
 * @{
 * @ingroup nrf_nfct
 * @brief   Near Field Communication Tag (NFCT) peripheral driver.
 */

#define NRFX_NFCT_NFCID1_SINGLE_SIZE 4u  ///< Length of single-size NFCID1.
#define NRFX_NFCT_NFCID1_DOUBLE_SIZE 7u  ///< Length of double-size NFCID1.
#define NRFX_NFCT_NFCID1_TRIPLE_SIZE 10u ///< Length of triple-size NFCID1.

#define NRFX_NFCT_NFCID1_DEFAULT_LEN NRFX_NFCT_NFCID1_DOUBLE_SIZE ///< Default length of NFC ID. */

/** @brief NFCT hardware states. */
typedef enum
{
    NRFX_NFCT_STATE_DISABLED  = NRF_NFCT_TASK_DISABLE,  ///< NFC Tag is disabled (no sensing of an external NFC field).
    NRFX_NFCT_STATE_SENSING   = NRF_NFCT_TASK_SENSE,    ///< NFC Tag is sensing whether there is an external NFC field.
    NRFX_NFCT_STATE_ACTIVATED = NRF_NFCT_TASK_ACTIVATE, ///< NFC Tag is powered-up (see @ref nrfx_nfct_active_state_t for possible substates).
} nrfx_nfct_state_t;

/**
 * @brief NFC tag states, when NFCT hardware is activated.
 *
 * @details These states are substates of the @ref NRFX_NFCT_STATE_ACTIVATED state.
 */
typedef enum
{
    NRFX_NFCT_ACTIVE_STATE_IDLE  = NRF_NFCT_TASK_GOIDLE,  ///< NFC Tag is activated and idle (not selected by a reader).
    NRFX_NFCT_ACTIVE_STATE_SLEEP = NRF_NFCT_TASK_GOSLEEP, ///< NFC Tag is sleeping.
    NRFX_NFCT_ACTIVE_STATE_DEFAULT,                       ///< NFC Tag is either sleeping or idle, depending on the previous state before being selected by a poller.
} nrfx_nfct_active_state_t;

/**
 * @brief NFCT driver event types, passed to the upper-layer callback function
 *        provided during the initialization.
 */
typedef enum
{
    NRFX_NFCT_EVT_FIELD_DETECTED = NRF_NFCT_INT_FIELDDETECTED_MASK, ///< External NFC field is detected.
    NRFX_NFCT_EVT_FIELD_LOST     = NRF_NFCT_INT_FIELDLOST_MASK,     ///< External NFC Field is lost.
    NRFX_NFCT_EVT_SELECTED       = NRF_NFCT_INT_SELECTED_MASK,      ///< Tag was selected by the poller.
    NRFX_NFCT_EVT_RX_FRAMESTART  = NRF_NFCT_INT_RXFRAMESTART_MASK,  ///< Data frame reception started.
    NRFX_NFCT_EVT_RX_FRAMEEND    = NRF_NFCT_INT_RXFRAMEEND_MASK,    ///< Data frame is received.
    NRFX_NFCT_EVT_TX_FRAMESTART  = NRF_NFCT_INT_TXFRAMESTART_MASK,  ///< Data frame transmission started.
    NRFX_NFCT_EVT_TX_FRAMEEND    = NRF_NFCT_INT_TXFRAMEEND_MASK,    ///< Data frame is transmitted.
    NRFX_NFCT_EVT_ERROR          = NRF_NFCT_INT_ERROR_MASK,         ///< Error occurred in an NFC communication.
} nrfx_nfct_evt_id_t;

/** @brief NFCT timing-related error types. */
typedef enum
{
    NRFX_NFCT_ERROR_FRAMEDELAYTIMEOUT, ///< No response frame was transmitted to the poller in the transmit window.
    NRFX_NFCT_ERROR_NUM,               ///< Total number of possible errors.
} nrfx_nfct_error_t;

/** @brief NFCT driver parameter types. */
typedef enum
{
    NRFX_NFCT_PARAM_ID_FDT,     ///< NFC-A Frame Delay Time parameter.
    NRFX_NFCT_PARAM_ID_SEL_RES, ///< Value of the 'Protocol' field in the NFC-A SEL_RES frame.
    NRFX_NFCT_PARAM_ID_NFCID1,  ///< NFC-A NFCID1 setting (NFC tag identifier).
} nrfx_nfct_param_id_t;

/** @brief NFCID1 descriptor. */
typedef struct
{
    uint8_t const * p_id;    ///< NFCID1 data.
    uint8_t         id_size; ///< NFCID1 size.
} nrfx_nfct_nfcid1_t;

/** @brief NFCT driver parameter descriptor. */
typedef struct
{
    nrfx_nfct_param_id_t   id;               ///< Type of parameter.
    union
    {
        uint32_t           fdt;              ///< NFC-A Frame Delay Time. Filled when nrfx_nfct_param_t::id is @ref NRFX_NFCT_PARAM_ID_FDT.
        uint8_t            sel_res_protocol; ///< NFC-A value of the 'Protocol' field in the SEL_RES frame. Filled when nrfx_nfct_param_t::id is @ref NRFX_NFCT_PARAM_ID_SEL_RES.
        nrfx_nfct_nfcid1_t nfcid1;           ///< NFC-A NFCID1 value (tag identifier). Filled when nrfx_nfct_param_t::id is @ref NRFX_NFCT_PARAM_ID_NFCID1.
    } data;                                  ///< Union to store parameter data.
} nrfx_nfct_param_t;

/** @brief NFCT driver RX/TX buffer descriptor. */
typedef struct
{
    uint32_t        data_size; ///< RX/TX buffer size.
    uint8_t const * p_data;    ///< RX/TX buffer.
} nrfx_nfct_data_desc_t;

/** @brief Structure used to describe the @ref NRFX_NFCT_EVT_RX_FRAMEEND event type. */
typedef struct
{
    uint32_t              rx_status; ///< RX error status.
    nrfx_nfct_data_desc_t rx_data;   ///< RX buffer.
} nrfx_nfct_evt_rx_frameend_t;

/** @brief Structure used to describe the @ref NRFX_NFCT_EVT_TX_FRAMESTART event type. */
typedef struct
{
    nrfx_nfct_data_desc_t tx_data; ///< TX buffer.
} nrfx_nfct_evt_tx_framestart_t;

/** @brief Structure used to describe the @ref NRFX_NFCT_EVT_ERROR event type. */
typedef struct
{
    nrfx_nfct_error_t reason; ///< Reason for error.
} nrfx_nfct_evt_error_t;

/** @brief NFCT driver event. */
typedef struct
{
    nrfx_nfct_evt_id_t evt_id;                       ///< Type of event.
    union
    {
        nrfx_nfct_evt_rx_frameend_t   rx_frameend;   ///< End of the RX frame data. Filled when nrfx_nfct_evt_t::evt_id is @ref NRFX_NFCT_EVT_RX_FRAMEEND.
        nrfx_nfct_evt_tx_framestart_t tx_framestart; ///< Start of the TX frame data. Filled when nrfx_nfct_evt_t::evt_id is @ref NRFX_NFCT_EVT_TX_FRAMESTART.
        nrfx_nfct_evt_error_t         error;         ///< Error data. Filled when nrfx_nfct_evt_t::evt_id is @ref NRFX_NFCT_EVT_ERROR.
    } params;                                        ///< Union to store event data.
} nrfx_nfct_evt_t;

/**
 * @brief Callback descriptor to pass events from the NFCT driver to the upper layer.
 *
 * @param[in] p_event Pointer to the event descriptor.
 */
typedef void (*nrfx_nfct_handler_t)(nrfx_nfct_evt_t const * p_event);

/** @brief NFCT driver configuration structure. */
typedef struct
{
    uint32_t            rxtx_int_mask; ///< Mask for enabling RX/TX events. Indicate which events must be forwarded to the upper layer by using @ref nrfx_nfct_evt_id_t. By default, no events are enabled. */
    nrfx_nfct_handler_t cb;            ///< Callback.
} nrfx_nfct_config_t;

/**
 * @brief Function for initializing the NFCT driver.
 *
 * @param[in] p_config  Pointer to the NFCT driver configuration structure.
 *
 * @retval NRFX_SUCCESS             The NFCT driver was initialized successfully.
 * @retval NRFX_ERROR_INVALID_STATE The NFCT driver is already initialized.
 */
nrfx_err_t nrfx_nfct_init(nrfx_nfct_config_t const * p_config);

/**
 * @brief Function for uninitializing the NFCT driver.
 *
 * After uninitialization, the instance is in disabled state.
 */
void nrfx_nfct_uninit(void);

/**
 * @brief Function for starting the NFC subsystem.
 *
 * After this function completes, NFC readers are able to detect the tag.
 */
void nrfx_nfct_enable(void);

/**
 * @brief Function for disabling the NFCT driver.
 *
 * After this function returns, NFC readers are no longer able to connect
 * to the tag.
 */
void nrfx_nfct_disable(void);

/**
 * @brief Function for checking whether the external NFC field is present in the range of the tag.
 *
 * @retval true  The NFC field is present.
 * @retval false No NFC field is present.
 */
bool nrfx_nfct_field_check(void);

/**
 * @brief Function for preparing the NFCT driver for receiving an NFC frame.
 *
 * @param[in] p_rx_data  Pointer to the RX buffer.
 */
void nrfx_nfct_rx(nrfx_nfct_data_desc_t const * p_rx_data);

/**
 * @brief Function for transmitting an NFC frame.
 *
 * @param[in] p_tx_data   Pointer to the TX buffer.
 * @param[in] delay_mode  Delay mode of the NFCT frame timer.
 *
 * @retval NRFX_SUCCESS              The operation was successful.
 * @retval NRFX_ERROR_INVALID_LENGTH The TX buffer size is invalid.
 */
nrfx_err_t nrfx_nfct_tx(nrfx_nfct_data_desc_t const * p_tx_data,
                        nrf_nfct_frame_delay_mode_t   delay_mode);

/**
 * @brief Function for moving the NFCT to a new state.
 *
 * @note  The HFCLK must be running before activating the NFCT with
 *        @ref NRFX_NFCT_STATE_ACTIVATED.
 *
 * @param[in] state  The required state.
 */
void nrfx_nfct_state_force(nrfx_nfct_state_t state);

/**
 * @brief Function for moving the NFCT to a new initial substate within @ref NRFX_NFCT_STATE_ACTIVATED.
 *
 * @param[in] sub_state  The required substate.
 */
void nrfx_nfct_init_substate_force(nrfx_nfct_active_state_t sub_state);

/**
 * @brief Function for setting the NFC communication parameter.
 *
 * @note Parameter validation for length and acceptable values.
 *
 * @param[in] p_param  Pointer to parameter descriptor.
 *
 * @retval NRFX_SUCCESS             The operation was successful.
 * @retval NRFX_ERROR_INVALID_PARAM The parameter data is invalid.
 */
nrfx_err_t nrfx_nfct_parameter_set(nrfx_nfct_param_t const * p_param);

/**
 * @brief Function for getting default bytes for NFCID1.
 *
 * @param[in,out] p_nfcid1_buff    In:  empty buffer for data;
 *                                 Out: buffer with the NFCID1 default data. These values
 *                                      can be used to fill the Type 2 Tag Internal Bytes.
 * @param[in]     nfcid1_buff_len  Length of the NFCID1 buffer.
 *
 * @retval NRFX_SUCCESS              The operation was successful.
 * @retval NRFX_ERROR_INVALID_LENGTH Length of the NFCID buffer is different than
 *                                   @ref NRFX_NFCT_NFCID1_SINGLE_SIZE,
 *                                   @ref NRFX_NFCT_NFCID1_DOUBLE_SIZE, or
 *                                   @ref NRFX_NFCT_NFCID1_TRIPLE_SIZE.
 */
nrfx_err_t nrfx_nfct_nfcid1_default_bytes_get(uint8_t * const p_nfcid1_buff,
                                              uint32_t        nfcid1_buff_len);

/**
 * @brief Function for enabling the automatic collision resolution.
 *
 * @details As defined by the NFC Forum Digital Protocol Technical Specification (and ISO 14443-3),
 *          the automatic collision resolution is implemented in the NFCT hardware.
 *          This function allows enabling and disabling this feature.
 */
void nrfx_nfct_autocolres_enable(void);

/**
 * @brief Function for disabling the automatic collision resolution.
 *
 * @details See also details in @ref nrfx_nfct_autocolres_enable.
 */
void nrfx_nfct_autocolres_disable(void);

/** @} */


void nrfx_nfct_irq_handler(void);


#ifdef __cplusplus
}
#endif


/**
 * @defgroup nrfx_nfct_fixes NFCT driver fixes and workarounds
 * @{
 * @ingroup nrf_nfct
 * @brief Fixes for hardware-related anomalies.
 *
 * If you are using the nRF52832 chip, the workarounds for the following anomalies are applied:
 * - 79. NFCT: A false EVENTS_FIELDDETECTED event occurs after the field is lost.
 * - 116. NFCT does not release HFCLK when switching from ACTIVATED to SENSE mode.
 * To implement the first workaround, an instance of NRF_TIMER is used. After the NFC field is detected,
 * the timing module periodically polls its state to determine when the field is turned off.
 * To implement the second workaround, power reset is used to release the clock acquired by NFCT
 * after the field is turned off. Note that the NFCT register configuration is restored to defaults.
 *
 * If you are using the nRF52840 chip, rev. Engineering A, the workarounds for the following anomalies
 * are applied:
 * - 98. NFCT: The NFCT is not able to communicate with the peer.
 * - 116. NFCT does not release HFCLK when switching from ACTIVATED to SENSE mode.
 * - 144. NFCT: Not optimal NFC performance
 *
 * If you are using the nRF52840 chip, rev. 1, or rev. Engineering B or C, the workarounds for the following
 * anomalies are applied:
 * - 190. NFCT: Event FIELDDETECTED can be generated too early.
 * To implement this workaround, an instance of NRF_TIMER is used. After the NFC field is detected,
 * the timing module measures the necessary waiting period after which NFCT can be activated.
 * This debouncing technique is used to filter possible field instabilities.
 *
 * The application of the implemented workarounds for the nRF52840 chip is determined at runtime and depends
 * on the chip variant.
 *
 * The current code contains a patch for the anomaly 25 (NFCT: Reset value of
 * SENSRES register is incorrect), so that the module now works on Windows Phone.
 * @}
 */

#endif // NRFX_NFCT_H__
