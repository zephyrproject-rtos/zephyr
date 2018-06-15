/*
 * Copyright (c) 2015 - 2018, Nordic Semiconductor ASA
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

#ifndef NRFX_SPIS_H__
#define NRFX_SPIS_H__

#include <nrfx.h>
#include <hal/nrf_spis.h>
#include <hal/nrf_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_spis SPI slave driver
 * @{
 * @ingroup nrf_spis
 * @brief   SPI Slave peripheral driver.
 */

/** @brief SPI slave driver instance data structure. */
typedef struct
{
    NRF_SPIS_Type * p_reg;          //!< Pointer to a structure with SPIS registers.
    uint8_t         drv_inst_idx;   //!< Driver instance index.
} nrfx_spis_t;

enum {
#if NRFX_CHECK(NRFX_SPIS0_ENABLED)
    NRFX_SPIS0_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_SPIS1_ENABLED)
    NRFX_SPIS1_INST_IDX,
#endif
#if NRFX_CHECK(NRFX_SPIS2_ENABLED)
    NRFX_SPIS2_INST_IDX,
#endif
    NRFX_SPIS_ENABLED_COUNT
};

/** @brief Macro for creating an SPI slave driver instance. */
#define NRFX_SPIS_INSTANCE(id)                               \
{                                                            \
    .p_reg        = NRFX_CONCAT_2(NRF_SPIS, id),             \
    .drv_inst_idx = NRFX_CONCAT_3(NRFX_SPIS, id, _INST_IDX), \
}

/**
 * @brief This value can be provided instead of a pin number for the signals MOSI
 *        and MISO to specify that the given signal is not used and therefore
 *        does not need to be connected to a pin.
 */
#define NRFX_SPIS_PIN_NOT_USED  0xFF

/** @brief Default pull-up configuration of the SPI CS. */
#define NRFX_SPIS_DEFAULT_CSN_PULLUP  NRF_GPIO_PIN_NOPULL
/** @brief Default drive configuration of the SPI MISO. */
#define NRFX_SPIS_DEFAULT_MISO_DRIVE  NRF_GPIO_PIN_S0S1

/** @brief SPI slave driver event types. */
typedef enum
{
    NRFX_SPIS_BUFFERS_SET_DONE, //!< Memory buffer set event. Memory buffers have been set successfully to the SPI slave device, and SPI transaction can be done.
    NRFX_SPIS_XFER_DONE,        //!< SPI transaction event. SPI transaction has been completed.
    NRFX_SPIS_EVT_TYPE_MAX      //!< Enumeration upper bound.
} nrfx_spis_evt_type_t;

/** @brief SPI slave driver event structure. */
typedef struct
{
    nrfx_spis_evt_type_t evt_type;  //!< Type of the event.
    size_t               rx_amount; //!< Number of bytes received in the last transaction. This parameter is only valid for @ref NRFX_SPIS_XFER_DONE events.
    size_t               tx_amount; //!< Number of bytes transmitted in the last transaction. This parameter is only valid for @ref NRFX_SPIS_XFER_DONE events.
} nrfx_spis_evt_t;

/** @brief SPI slave instance default configuration. */
#define NRFX_SPIS_DEFAULT_CONFIG                           \
{                                                          \
    .sck_pin      = NRFX_SPIS_PIN_NOT_USED,                \
    .mosi_pin     = NRFX_SPIS_PIN_NOT_USED,                \
    .miso_pin     = NRFX_SPIS_PIN_NOT_USED,                \
    .csn_pin      = NRFX_SPIS_PIN_NOT_USED,                \
    .mode         = NRF_SPIS_MODE_0,                       \
    .bit_order    = NRF_SPIS_BIT_ORDER_MSB_FIRST,          \
    .csn_pullup   = NRFX_SPIS_DEFAULT_CSN_PULLUP,          \
    .miso_drive   = NRFX_SPIS_DEFAULT_MISO_DRIVE,          \
    .def          = NRFX_SPIS_DEFAULT_DEF,                 \
    .orc          = NRFX_SPIS_DEFAULT_ORC,                 \
    .irq_priority = NRFX_SPIS_DEFAULT_CONFIG_IRQ_PRIORITY, \
}

/** @brief SPI peripheral device configuration data. */
typedef struct
{
    uint32_t             miso_pin;      //!< SPI MISO pin (optional).
                                        /**< Set @ref NRFX_SPIS_PIN_NOT_USED
                                         *   if this signal is not needed. */
    uint32_t             mosi_pin;      //!< SPI MOSI pin (optional).
                                        /**< Set @ref NRFX_SPIS_PIN_NOT_USED
                                         *   if this signal is not needed. */
    uint32_t             sck_pin;       //!< SPI SCK pin.
    uint32_t             csn_pin;       //!< SPI CSN pin.
    nrf_spis_mode_t      mode;          //!< SPI mode.
    nrf_spis_bit_order_t bit_order;     //!< SPI transaction bit order.
    nrf_gpio_pin_pull_t  csn_pullup;    //!< CSN pin pull-up configuration.
    nrf_gpio_pin_drive_t miso_drive;    //!< MISO pin drive configuration.
    uint8_t              def;           //!< Character clocked out in case of an ignored transaction.
    uint8_t              orc;           //!< Character clocked out after an over-read of the transmit buffer.
    uint8_t              irq_priority;  //!< Interrupt priority.
} nrfx_spis_config_t;


/**
 * @brief SPI slave driver event handler type.
 *
 * @param[in] p_event    Pointer to the event structure. The structure is
 *                       allocated on the stack so it is valid only until
 *                       the event handler returns.
 * @param[in] p_context  Context set on initialization.
 */
typedef void (*nrfx_spis_event_handler_t)(nrfx_spis_evt_t const * p_event,
                                          void *                  p_context);

/**
 * @brief Function for initializing the SPI slave driver instance.
 *
 * @note When the nRF52 Anomaly 109 workaround for SPIS is enabled, this function
 *       initializes the GPIOTE driver as well, and uses one of GPIOTE channels
 *       to detect falling edges on CSN pin.
 *
 * @param[in] p_instance    Pointer to the driver instance structure.
 * @param[in] p_config      Pointer to the structure with initial configuration.
 * @param[in] event_handler Function to be called by the SPI slave driver upon event.
 *                          Must not be NULL.
 * @param[in] p_context     Context passed to the event handler.
 *
 * @retval NRFX_SUCCESS             If the initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE If the instance is already initialized.
 * @retval NRFX_ERROR_INVALID_PARAM If an invalid parameter is supplied.
 * @retval NRFX_ERROR_BUSY          If some other peripheral with the same
 *                                  instance ID is already in use. This is
 *                                  possible only if @ref nrfx_prs module
 *                                  is enabled.
 * @retval NRFX_ERROR_INTERNAL      GPIOTE channel for detecting falling edges
 *                                  on CSN pin cannot be initialized. Possible
 *                                  only when using nRF52 Anomaly 109 workaround.
 */
nrfx_err_t nrfx_spis_init(nrfx_spis_t const * const  p_instance,
                          nrfx_spis_config_t const * p_config,
                          nrfx_spis_event_handler_t  event_handler,
                          void *                     p_context);

/**
 * @brief Function for uninitializing the SPI slave driver instance.
 *
 * @param[in] p_instance Pointer to the driver instance structure.
 */
void nrfx_spis_uninit(nrfx_spis_t const * const p_instance);

/**
 * @brief Function for preparing the SPI slave instance for a single SPI transaction.
 *
 * This function prepares the SPI slave device to be ready for a single SPI transaction. It configures
 * the SPI slave device to use the memory supplied with the function call in SPI transactions.
 *
 * When either the memory buffer configuration or the SPI transaction has been
 * completed, the event callback function will be called with the appropriate event
 * @ref nrfx_spis_evt_type_t. Note that the callback function can be called before returning from
 * this function, because it is called from the SPI slave interrupt context.
 *
 * @note This function can be called from the callback function context.
 *
 * @note Client applications must call this function after every @ref NRFX_SPIS_XFER_DONE event if
 * the SPI slave driver should be prepared for a possible new SPI transaction.
 *
 * @note Peripherals using EasyDMA (including SPIS) require the transfer buffers
 *       to be placed in the Data RAM region. If this condition is not met,
 *       this function will fail with the error code NRFX_ERROR_INVALID_ADDR.
 *
 * @param[in] p_instance            Pointer to the driver instance structure.
 * @param[in] p_tx_buffer           Pointer to the TX buffer. Can be NULL when the buffer length is zero.
 * @param[in] p_rx_buffer           Pointer to the RX buffer. Can be NULL when the buffer length is zero.
 * @param[in] tx_buffer_length      Length of the TX buffer in bytes.
 * @param[in] rx_buffer_length      Length of the RX buffer in bytes.
 *
 * @retval NRFX_SUCCESS              If the operation was successful.
 * @retval NRFX_ERROR_INVALID_STATE  If the operation failed because the SPI slave device is in an incorrect state.
 * @retval NRFX_ERROR_INVALID_ADDR   If the provided buffers are not placed in the Data
 *                                   RAM region.
 * @retval NRFX_ERROR_INVALID_LENGTH If provided lengths exceed the EasyDMA limits for the peripheral.
 * @retval NRFX_ERROR_INTERNAL       If the operation failed because of an internal error.
 */
nrfx_err_t nrfx_spis_buffers_set(nrfx_spis_t const * const p_instance,
                                 uint8_t const *           p_tx_buffer,
                                 size_t                    tx_buffer_length,
                                 uint8_t *                 p_rx_buffer,
                                 size_t                    rx_buffer_length);


void nrfx_spis_0_irq_handler(void);
void nrfx_spis_1_irq_handler(void);
void nrfx_spis_2_irq_handler(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_SPIS_H__

