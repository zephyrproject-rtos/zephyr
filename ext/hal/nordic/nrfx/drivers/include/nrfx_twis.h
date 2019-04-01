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

#ifndef NRFX_TWIS_H__
#define NRFX_TWIS_H__

#include <nrfx.h>
#include <hal/nrf_twis.h>
#include <hal/nrf_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_twis TWIS driver
 * @{
 * @ingroup nrf_twis
 * @brief   Two Wire Interface Slave with EasyDMA (TWIS) peripheral driver.
 */

/** @brief TWIS driver instance data structure. */
typedef struct
{
    NRF_TWIS_Type * p_reg;        ///< Pointer to a structure with TWIS registers.
    uint8_t         drv_inst_idx; ///< Index of the driver instance. For internal use only.
} nrfx_twis_t;

#ifndef __NRFX_DOXYGEN__
enum {
#if NRFX_CHECK(NRFX_TWIS0_ENABLED)
    NRFX_TWIS0_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_TWIS1_ENABLED)
    NRFX_TWIS1_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_TWIS2_ENABLED)
    NRFX_TWIS2_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_TWIS3_ENABLED)
    NRFX_TWIS3_INST_IDX,
#endif
    NRFX_TWIS_ENABLED_COUNT
};
#endif

/** @brief Macro for creating a TWIS driver instance. */
#define NRFX_TWIS_INSTANCE(id)                               \
{                                                            \
    .p_reg        = NRFX_CONCAT_2(NRF_TWIS, id),             \
    .drv_inst_idx = NRFX_CONCAT_3(NRFX_TWIS, id, _INST_IDX), \
}

/** @brief Event callback function event definitions. */
typedef enum
{
    NRFX_TWIS_EVT_READ_REQ,     ///< Read request detected.
                                /**< If there is no buffer prepared, buf_req flag in the even will be set.
                                     Call then @ref nrfx_twis_tx_prepare to give parameters for buffer.
                                     */
    NRFX_TWIS_EVT_READ_DONE,    ///< Read request finished - free any data.
    NRFX_TWIS_EVT_READ_ERROR,   ///< Read request finished with error.
    NRFX_TWIS_EVT_WRITE_REQ,    ///< Write request detected.
                                /**< If there is no buffer prepared, buf_req flag in the even will be set.
                                     Call then @ref nrfx_twis_rx_prepare to give parameters for buffer.
                                     */
    NRFX_TWIS_EVT_WRITE_DONE,   ///< Write request finished - process data.
    NRFX_TWIS_EVT_WRITE_ERROR,  ///< Write request finished with error.
    NRFX_TWIS_EVT_GENERAL_ERROR ///< Error that happens not inside WRITE or READ transaction.
} nrfx_twis_evt_type_t;

/**
 * @brief Possible error sources.
 *
 * This is flag enum - values from this enum can be connected using logical or operator.
 * @note
 * You can use directly @ref nrf_twis_error_t. Error type enum is redefined here because
 * of possible future extension (eg. supporting timeouts and synchronous mode).
 */
typedef enum
{
    NRFX_TWIS_ERROR_OVERFLOW         = NRF_TWIS_ERROR_OVERFLOW,  /**< RX buffer overflow detected, and prevented. */
    NRFX_TWIS_ERROR_DATA_NACK        = NRF_TWIS_ERROR_DATA_NACK, /**< NACK sent after receiving a data byte. */
    NRFX_TWIS_ERROR_OVERREAD         = NRF_TWIS_ERROR_OVERREAD,  /**< TX buffer over-read detected, and prevented. */
    NRFX_TWIS_ERROR_UNEXPECTED_EVENT = 1 << 8                    /**< Unexpected event detected by state machine. */
} nrfx_twis_error_t;

/** @brief TWIS driver event structure. */
typedef struct
{
    nrfx_twis_evt_type_t type; ///< Event type.
    union
    {
        bool buf_req;       ///< Flag for @ref NRFX_TWIS_EVT_READ_REQ and @ref NRFX_TWIS_EVT_WRITE_REQ.
                            /**< Information if transmission buffer requires to be prepared. */
        uint32_t tx_amount; ///< Data for @ref NRFX_TWIS_EVT_READ_DONE.
        uint32_t rx_amount; ///< Data for @ref NRFX_TWIS_EVT_WRITE_DONE.
        uint32_t error;     ///< Data for @ref NRFX_TWIS_EVT_GENERAL_ERROR.
    } data;                 ///< Union to store event data.
} nrfx_twis_evt_t;

/**
 * @brief TWI slave event callback function type.
 *
 * @param[in] p_event Event information structure.
 */
typedef void (*nrfx_twis_event_handler_t)(nrfx_twis_evt_t const * p_event);

/** @brief Structure for TWIS configuration. */
typedef struct
{
    uint32_t            addr[2];            //!< Set addresses that this slave should respond. Set 0 to disable.
    uint32_t            scl;                //!< SCL pin number.
    uint32_t            sda;                //!< SDA pin number.
    nrf_gpio_pin_pull_t scl_pull;           //!< SCL pin pull.
    nrf_gpio_pin_pull_t sda_pull;           //!< SDA pin pull.
    uint8_t             interrupt_priority; //!< The priority of interrupt for the module to be set.
} nrfx_twis_config_t;

/** @brief Generate the default configuration for the TWIS driver instance. */
#define NRFX_TWIS_DEFAULT_CONFIG                                                  \
{                                                                                 \
    .addr               = { NRFX_TWIS_DEFAULT_CONFIG_ADDR0,                       \
                            NRFX_TWIS_DEFAULT_CONFIG_ADDR1 },                     \
    .scl                = 31,                                                     \
    .sda                = 31,                                                     \
    .scl_pull           = (nrf_gpio_pin_pull_t)NRFX_TWIS_DEFAULT_CONFIG_SCL_PULL, \
    .sda_pull           = (nrf_gpio_pin_pull_t)NRFX_TWIS_DEFAULT_CONFIG_SDA_PULL, \
    .interrupt_priority = NRFX_TWIS_DEFAULT_CONFIG_IRQ_PRIORITY                   \
}

/**
 * @brief Function for initializing the TWIS driver instance.
 *
 * Function initializes and enables the TWIS driver.
 * @attention After driver initialization enable it with @ref nrfx_twis_enable.
 *
 * @param[in] p_instance    Pointer to the driver instance structure.
 * @attention               @em p_instance has to be global object.
 *                          It will be used by interrupts so make it sure that object
 *                          is not destroyed when function is leaving.
 * @param[in] p_config      Pointer to the structure with the initial configuration.
 * @param[in] event_handler Event handler provided by the user.
 *
 * @retval NRFX_SUCCESS             Initialization is successful.
 * @retval NRFX_ERROR_INVALID_STATE The driver is already initialized.
 * @retval NRFX_ERROR_BUSY          Some other peripheral with the same
 *                                  instance ID is already in use. This is
 *                                  possible only if NRFX_PRS_ENABLED
 *                                  is set to a value other than zero.
 */
nrfx_err_t nrfx_twis_init(nrfx_twis_t const *        p_instance,
                          nrfx_twis_config_t const * p_config,
                          nrfx_twis_event_handler_t  event_handler);

/**
 * @brief Function for uninitializing the TWIS driver instance.
 *
 * Function uninitializes the peripheral and resets all registers to default values.
 *
 * @note
 * It is safe to call nrfx_twis_uninit even before initialization.
 * Actually, @ref nrfx_twis_init function calls this function to
 * make sure that TWIS state is known.
 * @note
 * If TWIS driver was in uninitialized state before calling this function,
 * the selected pins would not be reset to default configuration.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_twis_uninit(nrfx_twis_t const * p_instance);

/**
 * @brief Function for enabling the TWIS instance.
 *
 * This function enables the TWIS instance.
 * Function defined if there is need for dynamically enabling and disabling the peripheral.
 * Use @ref nrfx_twis_enable and @ref nrfx_twis_disable functions.
 * They do not change any configuration registers.
 *
 * @param p_instance Pointer to the driver instance structure.
 */
void nrfx_twis_enable(nrfx_twis_t const * p_instance);

/**
 * @brief Function for disabling the TWIS instance.
 *
 * This function disables the TWIS instance, which gives possibility to turn off the TWIS while
 * holding configuration done by @ref nrfx_twis_init.
 *
 * @param p_instance Pointer to the driver instance structure.
 */
void nrfx_twis_disable(nrfx_twis_t const * p_instance);

/**
 * @brief Function for getting and clearing the last error flags.
 *
 * This function gets the information about errors.
 * This is also the only possibility to exit from the error substate of the internal state machine.
 * @attention
 * This function clears error state and flags.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @return Error flags defined in @ref nrfx_twis_error_t.
 */
uint32_t nrfx_twis_error_get_and_clear(nrfx_twis_t const * p_instance);

/**
 * @brief Function for preparing the data for sending.
 *
 * This function is to be used in response to the @ref NRFX_TWIS_EVT_READ_REQ event.
 *
 * @note Peripherals using EasyDMA (including TWIS) require the transfer buffers
 *       to be placed in the Data RAM region. If this condition is not met,
 *       this function will fail with the error code NRFX_ERROR_INVALID_ADDR.
 * @attention Transmission buffer must be placed in RAM.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_buf      Transmission buffer.
 * @param[in] size       Maximum number of bytes that master may read from buffer given.
 *
 * @retval NRFX_SUCCESS              The preparation finished properly.
 * @retval NRFX_ERROR_INVALID_ADDR   The given @em p_buf is not placed inside the RAM.
 * @retval NRFX_ERROR_INVALID_LENGTH There is a wrong value in the @em size parameter.
 * @retval NRFX_ERROR_INVALID_STATE  The module is not initialized or not enabled.
 */
nrfx_err_t nrfx_twis_tx_prepare(nrfx_twis_t const * p_instance,
                                void const *        p_buf,
                                size_t              size);

/**
 * @brief Function for getting the number of transmitted bytes.
 *
 * This function returns the number of bytes sent.
 * This function can be called after @ref NRFX_TWIS_EVT_READ_DONE or @ref NRFX_TWIS_EVT_READ_ERROR events.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @return Number of bytes sent.
 */
__STATIC_INLINE size_t nrfx_twis_tx_amount(nrfx_twis_t const * p_instance);

/**
 * @brief Function for preparing the data for receiving.
 *
 * This function must be used in response to the @ref NRFX_TWIS_EVT_WRITE_REQ event.
 *
 * @note Peripherals using EasyDMA (including TWIS) require the transfer buffers
 *       to be placed in the Data RAM region. If this condition is not met,
 *       this function fails with the error code NRFX_ERROR_INVALID_ADDR.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 * @param[in] p_buf      Buffer that is to be filled with received data.
 * @param[in] size       Size of the buffer (maximum amount of data to receive).
 *
 * @retval NRFX_SUCCESS              The preparation finished properly.
 * @retval NRFX_ERROR_INVALID_ADDR   The given @em p_buf is not placed inside the RAM.
 * @retval NRFX_ERROR_INVALID_LENGTH There is a wrong value in the @em size parameter.
 * @retval NRFX_ERROR_INVALID_STATE  The module is not initialized or not enabled.
 */
nrfx_err_t nrfx_twis_rx_prepare(nrfx_twis_t const * p_instance,
                                void *              p_buf,
                                size_t              size);

/**
 * @brief Function for getting the number of received bytes.
 *
 * This function returns number of bytes received.
 * It can be called after @ref NRFX_TWIS_EVT_WRITE_DONE or @ref NRFX_TWIS_EVT_WRITE_ERROR events.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @return Number of bytes received.
 */
__STATIC_INLINE size_t nrfx_twis_rx_amount(nrfx_twis_t const * p_instance);

/**
 * @brief Function for checking if the driver is busy right now.
 *
 * This function tests the actual driver substate.
 * If the driver is in any other state than IDLE or ERROR, this function returns true.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval true  The driver is in state other than ERROR or IDLE.
 * @retval false There is no transmission pending.
 */
bool nrfx_twis_is_busy(nrfx_twis_t const * p_instance);

/**
 * @brief Function for checking if the driver is waiting for a TX buffer.
 *
 * If this function returns true, the driver is stalled expecting
 * of the @ref nrfx_twis_tx_prepare function call.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval true  The driver is waiting for @ref nrfx_twis_tx_prepare.
 * @retval false The driver is not in the state where it is waiting for preparing a TX buffer.
 */
bool nrfx_twis_is_waiting_tx_buff(nrfx_twis_t const * p_instance);

/**
 * @brief Function for checking if the driver is waiting for an RX buffer.
 *
 * If this function returns true, the driver is stalled expecting
 * of the @ref nrfx_twis_rx_prepare function call.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval true  The driver is waiting for @ref nrfx_twis_rx_prepare.
 * @retval false The driver is not in the state where it is waiting for preparing an RX buffer.
 */
bool nrfx_twis_is_waiting_rx_buff(nrfx_twis_t const * p_instance);

/**
 * @brief Function for checking if the driver is sending data.
 *
 * If this function returns true, there is an ongoing output transmission.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval true  There is an ongoing output transmission.
 * @retval false The driver is in other state.
 */
bool nrfx_twis_is_pending_tx(nrfx_twis_t const * p_instance);

/**
 * @brief Function for checking if the driver is receiving data.
 *
 * If this function returns true, there is an ongoing input transmission.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 *
 * @retval true  There is an ongoing input transmission.
 * @retval false The driver is in other state.
 */
bool nrfx_twis_is_pending_rx(nrfx_twis_t const * p_instance);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION
__STATIC_INLINE size_t nrfx_twis_tx_amount(nrfx_twis_t const * p_instance)
{
    return nrf_twis_tx_amount_get(p_instance->p_reg);
}

__STATIC_INLINE size_t nrfx_twis_rx_amount(nrfx_twis_t const * p_instance)
{
    return nrf_twis_rx_amount_get(p_instance->p_reg);
}
#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */


void nrfx_twis_0_irq_handler(void);
void nrfx_twis_1_irq_handler(void);
void nrfx_twis_2_irq_handler(void);
void nrfx_twis_3_irq_handler(void);


#ifdef __cplusplus
}
#endif

#endif // NRFX_TWIS_H__
