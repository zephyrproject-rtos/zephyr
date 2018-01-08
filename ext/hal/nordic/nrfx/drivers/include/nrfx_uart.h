/**
 * Copyright (c) 2015 - 2017, Nordic Semiconductor ASA
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
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_UART_H__
#define NRFX_UART_H__

#include <nrfx.h>
#include <hal/nrf_uart.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_uart UART driver
 * @{
 * @ingroup nrf_uart
 * @brief   UART peripheral driver.
 */

/**
 * @brief UART driver instance data structure.
 */
typedef struct
{
    NRF_UART_Type * p_reg;        ///< Pointer to a structure with UART registers.
    uint8_t         drv_inst_idx; ///< Driver instance index.
} nrfx_uart_t;

enum {
#if NRFX_CHECK(NRFX_UART0_ENABLED)
    NRFX_UART0_INST_IDX,
#endif
    NRFX_UART_ENABLED_COUNT
};

/**
 * @brief Macro for creating a UART driver instance.
 */
#define NRFX_UART_INSTANCE(id)                               \
{                                                            \
    .p_reg        = NRFX_CONCAT_2(NRF_UART, id),             \
    .drv_inst_idx = NRFX_CONCAT_3(NRFX_UART, id, _INST_IDX), \
}

/**
 * @brief Types of UART driver events.
 */
typedef enum
{
    NRFX_UART_EVT_TX_DONE, ///< Requested TX transfer completed.
    NRFX_UART_EVT_RX_DONE, ///< Requested RX transfer completed.
    NRFX_UART_EVT_ERROR,   ///< Error reported by UART peripheral.
} nrfx_uart_evt_type_t;

/**
 * @brief Structure for UART configuration.
 */
typedef struct
{
    uint32_t            pseltxd;            ///< TXD pin number.
    uint32_t            pselrxd;            ///< RXD pin number.
    uint32_t            pselcts;            ///< CTS pin number.
    uint32_t            pselrts;            ///< RTS pin number.
    void *              p_context;          ///< Context passed to interrupt handler.
    nrf_uart_hwfc_t     hwfc;               ///< Flow control configuration.
    nrf_uart_parity_t   parity;             ///< Parity configuration.
    nrf_uart_baudrate_t baudrate;           ///< Baudrate.
    uint8_t             interrupt_priority; ///< Interrupt priority.
} nrfx_uart_config_t;

/**
 * @brief UART default configuration.
 */
#define NRFX_UART_DEFAULT_CONFIG                                                  \
{                                                                                 \
    .pseltxd            = NRF_UART_PSEL_DISCONNECTED,                             \
    .pselrxd            = NRF_UART_PSEL_DISCONNECTED,                             \
    .pselcts            = NRF_UART_PSEL_DISCONNECTED,                             \
    .pselrts            = NRF_UART_PSEL_DISCONNECTED,                             \
    .p_context          = NULL,                                                   \
    .hwfc               = (nrf_uart_hwfc_t)NRFX_UART_DEFAULT_CONFIG_HWFC,         \
    .parity             = (nrf_uart_parity_t)NRFX_UART_DEFAULT_CONFIG_PARITY,     \
    .baudrate           = (nrf_uart_baudrate_t)NRFX_UART_DEFAULT_CONFIG_BAUDRATE, \
    .interrupt_priority = NRFX_UART_DEFAULT_CONFIG_IRQ_PRIORITY,                  \
}

/**
 * @brief Structure for UART transfer completion event.
 */
typedef struct
{
    uint8_t * p_data; ///< Pointer to memory used for transfer.
    uint32_t  bytes;  ///< Number of bytes transfered.
} nrfx_uart_xfer_evt_t;

/**
 * @brief Structure for UART error event.
 */
typedef struct
{
    nrfx_uart_xfer_evt_t rxtx;       ///< Transfer details includes number of bytes transferred.
    uint32_t             error_mask; ///< Mask of error flags that generated the event.
} nrfx_uart_error_evt_t;

/**
 * @brief Structure for UART event.
 */
typedef struct
{
    nrfx_uart_evt_type_t type; ///< Event type.
    union
    {
        nrfx_uart_xfer_evt_t  rxtx;  ///< Data provided for transfer completion events.
        nrfx_uart_error_evt_t error; ///< Data provided for error event.
    } data;
} nrfx_uart_event_t;

/**
 * @brief UART interrupt event handler.
 *
 * @param[in] p_event    Pointer to event structure. Event is allocated on the stack so it is available
 *                       only within the context of the event handler.
 * @param[in] p_context  Context passed to interrupt handler, set on initialization.
 */
typedef void (*nrfx_uart_event_handler_t)(nrfx_uart_event_t const * p_event,
                                          void *                    p_context);

/**
 * @brief Function for initializing the UART driver.
 *
 * This function configures and enables UART. After this function GPIO pins are controlled by UART.
 *
 * @param[in] p_instance    Pointer to the driver instance structure.
 * @param[in] p_config      Pointer to the structure with initial configuration.
 * @param[in] event_handler Event handler provided by the user. If not provided driver works in
 *                          blocking mode.
 *
 * @retval    NRFX_SUCCESS             If initialization was successful.
 * @retval    NRFX_ERROR_INVALID_STATE If driver is already initialized.
 * @retval    NRFX_ERROR_BUSY          If some other peripheral with the same
 *                                     instance ID is already in use. This is
 *                                     possible only if @ref nrfx_prs module
 *                                     is enabled.
 */
nrfx_err_t nrfx_uart_init(nrfx_uart_t const *        p_instance,
                          nrfx_uart_config_t const * p_config,
                          nrfx_uart_event_handler_t  event_handler);

/**
 * @brief Function for uninitializing  the UART driver.
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_uart_uninit(nrfx_uart_t const * p_instance);

/**
 * @brief Function for getting the address of a specific UART task.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] task       Task.
 *
 * @return    Task address.
 */
__STATIC_INLINE uint32_t nrfx_uart_task_address_get(nrfx_uart_t const * p_instance,
                                                    nrf_uart_task_t     task);

/**
 * @brief Function for getting the address of a specific UART event.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] event      Event.
 *
 * @return    Event address.
 */
__STATIC_INLINE uint32_t nrfx_uart_event_address_get(nrfx_uart_t const * p_instance,
                                                     nrf_uart_event_t    event);

/**
 * @brief Function for sending data over UART.
 *
 * If an event handler was provided in nrfx_uart_init() call, this function
 * returns immediately and the handler is called when the transfer is done.
 * Otherwise, the transfer is performed in blocking mode, i.e. this function
 * returns when the transfer is finished. Blocking mode is not using interrupt
 * so there is no context switching inside the function.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_data     Pointer to data.
 * @param[in] length     Number of bytes to send.
 *
 * @retval    NRFX_SUCCESS            If initialization was successful.
 * @retval    NRFX_ERROR_BUSY         If driver is already transferring.
 * @retval    NRFX_ERROR_FORBIDDEN    If the transfer was aborted from a different context
 *                                    (blocking mode only).
 */
nrfx_err_t nrfx_uart_tx(nrfx_uart_t const * p_instance,
                        uint8_t const *     p_data,
                        size_t              length);

/**
 * @brief Function for checking if UART is currently transmitting.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval true  If UART is transmitting.
 * @retval false If UART is not transmitting.
 */
bool nrfx_uart_tx_in_progress(nrfx_uart_t const * p_instance);

/**
 * @brief Function for aborting any ongoing transmission.
 * @note @ref NRFX_UART_EVT_TX_DONE event will be generated in non-blocking mode.
 *       It will contain number of bytes sent until abort was called. The event
 *       handler will be called from the function context.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_uart_tx_abort(nrfx_uart_t const * p_instance);

/**
 * @brief Function for receiving data over UART.
 *
 * If an event handler was provided in the nrfx_uart_init() call, this function
 * returns immediately and the handler is called when the transfer is done.
 * Otherwise, the transfer is performed in blocking mode, i.e. this function
 * returns when the transfer is finished. Blocking mode is not using interrupt so
 * there is no context switching inside the function.
 * The receive buffer pointer is double buffered in non-blocking mode. The secondary
 * buffer can be set immediately after starting the transfer and will be filled
 * when the primary buffer is full. The double buffering feature allows
 * receiving data continuously.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_data     Pointer to data.
 * @param[in] length     Number of bytes to receive.
 *
 * @retval    NRFX_SUCCESS If initialization was successful.
 * @retval    NRFX_ERROR_BUSY If the driver is already receiving
 *                            (and the secondary buffer has already been set
 *                            in non-blocking mode).
 * @retval    NRFX_ERROR_FORBIDDEN If the transfer was aborted from a different context
 *                                (blocking mode only, also see @ref nrfx_uart_rx_disable).
 * @retval    NRFX_ERROR_INTERNAL If UART peripheral reported an error.
 */
nrfx_err_t nrfx_uart_rx(nrfx_uart_t const * p_instance,
                        uint8_t *           p_data,
                        size_t              length);



/**
 * @brief Function for testing the receiver state in blocking mode.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval true  If the receiver has at least one byte of data to get.
 * @retval false If the receiver is empty.
 */
bool nrfx_uart_rx_ready(nrfx_uart_t const * p_instance);

/**
 * @brief Function for enabling the receiver.
 *
 * UART has a 6-byte-long RX FIFO and it is used to store incoming data. If a user does not call the
 * UART receive function before the FIFO is filled, an overrun error will appear. The receiver must be
 * explicitly closed by the user @sa nrfx_uart_rx_disable.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_uart_rx_enable(nrfx_uart_t const * p_instance);

/**
 * @brief Function for disabling the receiver.
 *
 * This function must be called to close the receiver after it has been explicitly enabled by
 * @sa nrfx_uart_rx_enable.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_uart_rx_disable(nrfx_uart_t const * p_instance);

/**
 * @brief Function for aborting any ongoing reception.
 * @note @ref NRFX_UART_EVT_TX_DONE event will be generated in non-blocking mode.
 *       It will contain number of bytes received until abort was called. The event
 *       handler will be called from the UART interrupt context.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_uart_rx_abort(nrfx_uart_t const * p_instance);

/**
 * @brief Function for reading error source mask. Mask contains values from @ref nrf_uart_error_mask_t.
 * @note Function should be used in blocking mode only. In case of non-blocking mode, an error event is
 *       generated. Function clears error sources after reading.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval    Mask of reported errors.
 */
uint32_t nrfx_uart_errorsrc_get(nrfx_uart_t const * p_instance);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION
__STATIC_INLINE uint32_t nrfx_uart_task_address_get(nrfx_uart_t const * p_instance,
                                                    nrf_uart_task_t     task)
{
    return nrf_uart_task_address_get(p_instance->p_reg, task);
}

__STATIC_INLINE uint32_t nrfx_uart_event_address_get(nrfx_uart_t const * p_instance,
                                                     nrf_uart_event_t    event)
{
    return nrf_uart_event_address_get(p_instance->p_reg, event);
}
#endif // SUPPRESS_INLINE_IMPLEMENTATION


void nrfx_uart_0_irq_handler(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_UART_H__
