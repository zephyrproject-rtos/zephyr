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

#ifndef NRFX_SPIM_H__
#define NRFX_SPIM_H__

#include <nrfx.h>
#include <hal/nrf_spim.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_spim SPIM driver
 * @{
 * @ingroup nrf_spim
 * @brief   SPIM peripheral driver.
 */

/**
 * @brief SPIM master driver instance data structure.
 */
typedef struct
{
    NRF_SPIM_Type * p_reg;        ///< Pointer to a structure with SPIM registers.
    uint8_t         drv_inst_idx; ///< Driver instance index.
} nrfx_spim_t;

enum {
#if NRFX_CHECK(NRFX_SPIM0_ENABLED)
    NRFX_SPIM0_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_SPIM1_ENABLED)
    NRFX_SPIM1_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_SPIM2_ENABLED)
    NRFX_SPIM2_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_SPIM3_ENABLED)
    NRFX_SPIM3_INST_IDX,
#endif
    NRFX_SPIM_ENABLED_COUNT
};

/**
 * @brief Macro for creating an SPIM master driver instance.
 */
#define NRFX_SPIM_INSTANCE(id)                               \
{                                                            \
    .p_reg        = NRFX_CONCAT_2(NRF_SPIM, id),             \
    .drv_inst_idx = NRFX_CONCAT_3(NRFX_SPIM, id, _INST_IDX), \
}

/**
 * @brief This value can be provided instead of a pin number for signals MOSI,
 *        MISO, and Slave Select to specify that the given signal is not used and
 *        therefore does not need to be connected to a pin.
 */
#define NRFX_SPIM_PIN_NOT_USED  0xFF

/**
 * @brief SPIM master driver instance configuration structure.
 */
typedef struct
{
    uint8_t sck_pin;      ///< SCK pin number.
    uint8_t mosi_pin;     ///< MOSI pin number (optional).
                          /**< Set to @ref NRFX_SPIM_PIN_NOT_USED
                           *   if this signal is not needed. */
    uint8_t miso_pin;     ///< MISO pin number (optional).
                          /**< Set to @ref NRFX_SPIM_PIN_NOT_USED
                           *   if this signal is not needed. */
    uint8_t ss_pin;       ///< Slave Select pin number (optional).
                          /**< Set to @ref NRFX_SPIM_PIN_NOT_USED
                           *   if this signal is not needed. */
    bool ss_active_high;  ///< Polarity of the Slave Select pin during transmission.
    uint8_t irq_priority; ///< Interrupt priority.
    uint8_t orc;          ///< Over-run character.
                          /**< This character is used when all bytes from the TX buffer are sent,
                               but the transfer continues due to RX. */
    nrf_spim_frequency_t frequency; ///< SPI frequency.
    nrf_spim_mode_t      mode;      ///< SPI mode.
    nrf_spim_bit_order_t bit_order; ///< SPI bit order.
#if NRFX_CHECK(NRFX_SPIM_EXTENDED_ENABLED) || defined(__NRFX_DOXYGEN__)
    uint8_t              dcx_pin;     ///< D/CX pin number (optional).
    uint8_t              rx_delay;    ///< Sample delay for input serial data on MISO.
                                      /**< The value specifies the delay, in number of 64 MHz clock cycles
                                       *   (15.625 ns), from the the sampling edge of SCK (leading edge for
                                       *   CONFIG.CPHA = 0, trailing edge for CONFIG.CPHA = 1) until
                                       *   the input serial data is sampled.*/
    bool                 use_hw_ss;   ///< Indication to use software or hardware controlled Slave Select pin.
    uint8_t              ss_duration; ///< Slave Select duration before and after transmission.
                                      /**< Minimum duration between the edge of CSN and the edge of SCK and minimum
                                       *   duration of CSN must stay inactive between transactions.
                                       *   The value is specified in number of 64 MHz clock cycles (15.625 ns).
                                       *   Supported only for hardware controlled Slave Select.*/
#endif
} nrfx_spim_config_t;

#if NRFX_CHECK(NRFX_SPIM_EXTENDED_ENABLED) || defined(__NRFX_DOXYGEN__)
/**
 * @brief SPIM master instance extended default configuration.
 */
    #define NRFX_SPIM_DEFAULT_EXTENDED_CONFIG   \
        .dcx_pin      = NRFX_SPIM_PIN_NOT_USED, \
        .rx_delay     = 0x02,                   \
        .ss_duration  = 0x02,                   \
        .use_hw_ss    = false,
#else
    #define NRFX_SPIM_DEFAULT_EXTENDED_CONFIG
#endif

/**
 * @brief SPIM master instance default configuration.
 */
#define NRFX_SPIM_DEFAULT_CONFIG                             \
{                                                            \
    .sck_pin        = NRFX_SPIM_PIN_NOT_USED,                \
    .mosi_pin       = NRFX_SPIM_PIN_NOT_USED,                \
    .miso_pin       = NRFX_SPIM_PIN_NOT_USED,                \
    .ss_pin         = NRFX_SPIM_PIN_NOT_USED,                \
    .ss_active_high = false,                                 \
    .irq_priority   = NRFX_SPIM_DEFAULT_CONFIG_IRQ_PRIORITY, \
    .orc            = 0xFF,                                  \
    .frequency      = NRF_SPIM_FREQ_4M,                      \
    .mode           = NRF_SPIM_MODE_0,                       \
    .bit_order      = NRF_SPIM_BIT_ORDER_MSB_FIRST,          \
    NRFX_SPIM_DEFAULT_EXTENDED_CONFIG                        \
}

#define NRFX_SPIM_FLAG_TX_POSTINC          (1UL << 0) /**< TX buffer address incremented after transfer. */
#define NRFX_SPIM_FLAG_RX_POSTINC          (1UL << 1) /**< RX buffer address incremented after transfer. */
#define NRFX_SPIM_FLAG_NO_XFER_EVT_HANDLER (1UL << 2) /**< Interrupt after each transfer is suppressed, and the event handler is not called. */
#define NRFX_SPIM_FLAG_HOLD_XFER           (1UL << 3) /**< Set up the transfer but do not start it. */
#define NRFX_SPIM_FLAG_REPEATED_XFER       (1UL << 4) /**< Flag indicating that the transfer will be executed multiple times. */

/**
 * @brief Single transfer descriptor structure.
 */
typedef struct
{
    uint8_t const * p_tx_buffer; ///< Pointer to TX buffer.
    size_t          tx_length;   ///< TX buffer length.
    uint8_t       * p_rx_buffer; ///< Pointer to RX buffer.
    size_t          rx_length;   ///< RX buffer length.
} nrfx_spim_xfer_desc_t;

/**
 * @brief Macro for setting up single transfer descriptor.
 *
 * This macro is for internal use only.
 */
#define NRFX_SPIM_SINGLE_XFER(p_tx, tx_len, p_rx, rx_len)    \
    {                                                        \
    .p_tx_buffer = (uint8_t const *)(p_tx),                  \
    .tx_length = (tx_len),                                   \
    .p_rx_buffer = (p_rx),                                   \
    .rx_length = (rx_len),                                   \
    }

/**
 * @brief Macro for setting duplex TX RX transfer.
 */
#define NRFX_SPIM_XFER_TRX(p_tx_buf, tx_length, p_rx_buf, rx_length)                    \
        NRFX_SPIM_SINGLE_XFER(p_tx_buf, tx_length, p_rx_buf, rx_length)

/**
 * @brief Macro for setting TX transfer.
 */
#define NRFX_SPIM_XFER_TX(p_buf, length) \
        NRFX_SPIM_SINGLE_XFER(p_buf, length, NULL, 0)

/**
 * @brief Macro for setting RX transfer.
 */
#define NRFX_SPIM_XFER_RX(p_buf, length) \
        NRFX_SPIM_SINGLE_XFER(NULL, 0, p_buf, length)

/**
 * @brief SPIM master driver event types, passed to the handler routine provided
 *        during initialization.
 */
typedef enum
{
    NRFX_SPIM_EVENT_DONE, ///< Transfer done.
} nrfx_spim_evt_type_t;

typedef struct
{
    nrfx_spim_evt_type_t  type;      ///< Event type.
    nrfx_spim_xfer_desc_t xfer_desc; ///< Transfer details.
} nrfx_spim_evt_t;

/**
 * @brief SPIM master driver event handler type.
 */
typedef void (* nrfx_spim_evt_handler_t)(nrfx_spim_evt_t const * p_event,
                                         void *                  p_context);

/**
 * @brief Function for initializing the SPI master driver instance.
 *
 * This function configures and enables the specified peripheral.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_config   Pointer to the structure with initial configuration.
 *
 * @param     handler    Event handler provided by the user. If NULL, transfers
 *                       will be performed in blocking mode.
 * @param     p_context  Context passed to event handler.
 *
 * @retval NRFX_SUCCESS             If initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE If the driver was already initialized.
 * @retval NRFX_ERROR_BUSY          If some other peripheral with the same
 *                                  instance ID is already in use. This is
 *                                  possible only if @ref nrfx_prs module
 *                                  is enabled.
 * @retval NRFX_ERROR_NOT_SUPPORTED If requested configuration is not supported
 *                                  by the SPIM instance.
 */
nrfx_err_t nrfx_spim_init(nrfx_spim_t const * const  p_instance,
                          nrfx_spim_config_t const * p_config,
                          nrfx_spim_evt_handler_t    handler,
                          void *                     p_context);

/**
 * @brief Function for uninitializing the SPI master driver instance.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void       nrfx_spim_uninit(nrfx_spim_t const * const p_instance);

/**
 * @brief Function for starting the SPI data transfer.
 *
 * Additional options are provided using the @c flags parameter:
 *
 * - @ref NRFX_SPIM_FLAG_TX_POSTINC and @ref NRFX_SPIM_FLAG_RX_POSTINC<span></span>:
 *   Post-incrementation of buffer addresses. Supported only by SPIM.
 * - @ref NRFX_SPIM_FLAG_HOLD_XFER<span></span>: Driver is not starting the transfer. Use this
 *   flag if the transfer is triggered externally by PPI. Supported only by SPIM. Use
 *   @ref nrfx_spim_start_task_get to get the address of the start task.
 * - @ref NRFX_SPIM_FLAG_NO_XFER_EVT_HANDLER<span></span>: No user event handler after transfer
 *   completion. This also means no interrupt at the end of the transfer. Supported only by SPIM.
 *   If @ref NRFX_SPIM_FLAG_NO_XFER_EVT_HANDLER is used, the driver does not set the instance into
 *   busy state, so you must ensure that the next transfers are set up when SPIM is not active.
 *   @ref nrfx_spim_end_event_get function can be used to detect end of transfer. Option can be used
 *   together with @ref NRFX_SPIM_FLAG_REPEATED_XFER to prepare a sequence of SPI transfers
 *   without interruptions.
 * - @ref NRFX_SPIM_FLAG_REPEATED_XFER<span></span>: Prepare for repeated transfers. You can set
 *   up a number of transfers that will be triggered externally (for example by PPI). An example is
 *   a TXRX transfer with the options @ref NRFX_SPIM_FLAG_RX_POSTINC,
 *   @ref NRFX_SPIM_FLAG_NO_XFER_EVT_HANDLER, and @ref NRFX_SPIM_FLAG_REPEATED_XFER. After the
 *   transfer is set up, a set of transfers can be triggered by PPI that will read, for example,
 *   the same register of an external component and put it into a RAM buffer without any interrupts.
 *   @ref nrfx_spim_end_event_get can be used to get the address of the END event, which can be
 *   used to count the number of transfers. If @ref NRFX_SPIM_FLAG_REPEATED_XFER is used,
 *   the driver does not set the instance into busy state, so you must ensure that the next
 *   transfers are set up when SPIM is not active. Supported only by SPIM.
 *
 * @note Peripherals using EasyDMA (including SPIM) require the transfer buffers
 *       to be placed in the Data RAM region. If this condition is not met,
 *       this function will fail with the error code NRFX_ERROR_INVALID_ADDR.
 *
 * @param p_instance  Pointer to the driver instance structure.
 * @param p_xfer_desc Pointer to the transfer descriptor.
 * @param flags       Transfer options (0 for default settings).
 *
 * @retval NRFX_SUCCESS             If the procedure was successful.
 * @retval NRFX_ERROR_BUSY          If the driver is not ready for a new transfer.
 * @retval NRFX_ERROR_NOT_SUPPORTED If the provided parameters are not supported.
 * @retval NRFX_ERROR_INVALID_ADDR  If the provided buffers are not placed in the Data
 *                                  RAM region.
 */
nrfx_err_t nrfx_spim_xfer(nrfx_spim_t const * const     p_instance,
                          nrfx_spim_xfer_desc_t const * p_xfer_desc,
                          uint32_t                      flags);

#if NRFX_CHECK(NRFX_SPIM_EXTENDED_ENABLED) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for starting the SPI data transfer with DCX control.
 *
 * See @ref nrfx_spim_xfer for description of additional options of transfer
 * provided by the @c flags parameter.
 *
 * @note Peripherals that use EasyDMA (including SPIM) require the transfer buffers
 *       to be placed in the Data RAM region. If this condition is not met,
 *       this function will fail with the error code NRFX_ERROR_INVALID_ADDR.
 *
 * @param p_instance  Pointer to the driver instance structure.
 * @param p_xfer_desc Pointer to the transfer descriptor.
 * @param flags       Transfer options (0 for default settings).
 * @param cmd_length  Length of the command bytes preceding the data
 *                    bytes. The DCX line will be low during transmission
 *                    of command bytes and high during transmission of data bytes.
 *                    Maximum value available for dividing the transmitted bytes
 *                    into command bytes and data bytes is @ref NRF_SPIM_DCX_CNT_ALL_CMD - 1.
 *                    The @ref NRF_SPIM_DCX_CNT_ALL_CMD value passed as the
 *                    @c cmd_length parameter causes all transmitted bytes
 *                    to be marked as command bytes.
 *
 * @retval NRFX_SUCCESS              If the procedure was successful.
 * @retval NRFX_ERROR_BUSY           If the driver is not ready for a new transfer.
 * @retval NRFX_ERROR_NOT_SUPPORTED  If the provided parameters are not supported.
 * @retval NRFX_ERROR_INVALID_ADDR   If the provided buffers are not placed in the Data
 *                                   RAM region.
 */
nrfx_err_t nrfx_spim_xfer_dcx(nrfx_spim_t const * const     p_instance,
                              nrfx_spim_xfer_desc_t const * p_xfer_desc,
                              uint32_t                      flags,
                              uint8_t                       cmd_length);
#endif

/**
 * @brief Function for returning the address of a SPIM start task.
 *
 * This function should be used if @ref nrfx_spim_xfer was called with the flag @ref NRFX_SPIM_FLAG_HOLD_XFER.
 * In that case, the transfer is not started by the driver, but it must be started externally by PPI.
 *
 * @param[in]  p_instance Pointer to the driver instance structure.
 *
 * @return     Start task address.
 */
uint32_t nrfx_spim_start_task_get(nrfx_spim_t const * p_instance);

/**
 * @brief Function for returning the address of a END SPIM event.
 *
 * The END event can be used to detect the end of a transfer
 * if the @ref NRFX_SPIM_FLAG_NO_XFER_EVT_HANDLER option is used.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @return END event address.
 */
uint32_t nrfx_spim_end_event_get(nrfx_spim_t const * p_instance);

/**
 * @brief Function for aborting ongoing transfer.
 *
 * @param[in]  p_instance Pointer to the driver instance structure.
 */
void nrfx_spim_abort(nrfx_spim_t const * p_instance);


void nrfx_spim_0_irq_handler(void);
void nrfx_spim_1_irq_handler(void);
void nrfx_spim_2_irq_handler(void);
void nrfx_spim_3_irq_handler(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_SPIM_H__
