/**
 * Copyright (c) 2016 - 2017, Nordic Semiconductor ASA
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

#ifndef NRFX_QSPI_H__
#define NRFX_QSPI_H__

#include <nrfx.h>
#include <hal/nrf_qspi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_qspi QSPI driver
 * @{
 * @ingroup nrf_qspi
 * @brief   Quad Serial Peripheral Interface (QSPI) peripheral driver.
 */

/**
 * @brief QSPI driver instance configuration structure.
 */
typedef struct
{
    uint32_t             xip_offset;   /**< Address offset into the external memory for Execute in Place operation. */
    nrf_qspi_pins_t      pins;         /**< Pins configuration structure. */
    nrf_qspi_prot_conf_t prot_if;      /**< Protocol layer interface configuration structure. */
    nrf_qspi_phy_conf_t  phy_if;       /**< Physical layer interface configuration structure. */
    uint8_t              irq_priority; /**< Interrupt priority. */
} nrfx_qspi_config_t;

/**
 * @brief QSPI instance default configuration.
 */
#define NRFX_QSPI_DEFAULT_CONFIG                                        \
{                                                                       \
    .xip_offset  = NRFX_QSPI_CONFIG_XIP_OFFSET,                         \
    .pins = {                                                           \
       .sck_pin     = NRFX_QSPI_PIN_SCK,                                \
       .csn_pin     = NRFX_QSPI_PIN_CSN,                                \
       .io0_pin     = NRFX_QSPI_PIN_IO0,                                \
       .io1_pin     = NRFX_QSPI_PIN_IO1,                                \
       .io2_pin     = NRFX_QSPI_PIN_IO2,                                \
       .io3_pin     = NRFX_QSPI_PIN_IO3,                                \
    },                                                                  \
    .irq_priority   = (uint8_t)NRFX_QSPI_CONFIG_IRQ_PRIORITY,           \
    .prot_if = {                                                        \
        .readoc     = (nrf_qspi_readoc_t)NRFX_QSPI_CONFIG_READOC,       \
        .writeoc    = (nrf_qspi_writeoc_t)NRFX_QSPI_CONFIG_WRITEOC,     \
        .addrmode   = (nrf_qspi_addrmode_t)NRFX_QSPI_CONFIG_ADDRMODE,   \
        .dpmconfig  = false,                                            \
    },                                                                  \
    .phy_if = {                                                         \
        .sck_freq   = (nrf_qspi_frequency_t)NRFX_QSPI_CONFIG_FREQUENCY, \
        .sck_delay  = (uint8_t)NRFX_QSPI_CONFIG_SCK_DELAY,              \
        .spi_mode   = (nrf_qspi_spi_mode_t)NRFX_QSPI_CONFIG_MODE,       \
        .dpmen      = false                                             \
    },                                                                  \
}

/**
 * @brief QSPI custom instruction helper with default configuration.
 */
#define NRFX_QSPI_DEFAULT_CINSTR(opc, len) \
{                                          \
    .opcode    = (opc),                    \
    .length    = (len),                    \
    .io2_level = false,                    \
    .io3_level = false,                    \
    .wipwait   = false,                    \
    .wren      = false                     \
}

/**
 * @brief QSPI master driver event types, passed to the handler routine provided
 *        during initialization.
 */
typedef enum
{
    NRFX_QSPI_EVENT_DONE, /**< Transfer done. */
} nrfx_qspi_evt_t;

/**
 * @brief QSPI driver event handler type.
 */
typedef void (*nrfx_qspi_handler_t)(nrfx_qspi_evt_t event, void * p_context);

/**
 * @brief Function for initializing the QSPI driver instance.
 *
 * @param[in] p_config   Pointer to the structure with initial configuration.
 * @param[in] handler    Event handler provided by the user. If NULL, transfers
 *                       will be performed in blocking mode.
 * @param[in] p_context  Pointer to context. Use in interrupt handler.
 *
 *
 * @retval NRFX_SUCCESS             If initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE If the driver was already initialized.
 * @retval NRFX_ERROR_INVALID_PARAM If the pin configuration was incorrect.
 */
nrfx_err_t nrfx_qspi_init(nrfx_qspi_config_t const * p_config,
                          nrfx_qspi_handler_t        handler,
                          void *                     p_context);

/**
 * @brief Function for uninitializing the QSPI driver instance.
 */
void nrfx_qspi_uninit(void);

/**
 * @brief Function for reading data from QSPI memory.
 *
 * Write, read, and erase operations check memory device busy state before starting the operation.
 * If the memory is busy, the resulting action depends on the mode in which the read operation is used:
 *  - blocking mode (without handler) - a delay occurs until the last operation still runs and
 *    until operation data is still being read.
 *  - interrupt mode (with handler) - event emission occurs after the last operation
 *    and reading of data are finished.
 *
 * @param[out] p_rx_buffer      Pointer to the receive buffer.
 * @param[in]  rx_buffer_length Size of the data to read.
 * @param[in]  src_address      Address in memory to read from.
 *
 * @retval NRFX_SUCCESS            If the operation was successful (blocking mode) or operation
 *                                 was commissioned (handler mode).
 * @retval NRFX_ERROR_BUSY         If the driver currently handles another operation.
 * @retval NRFX_ERROR_INVALID_ADDR If the provided buffer is not placed in the Data RAM region.
 */
nrfx_err_t nrfx_qspi_read(void *   p_rx_buffer,
                          size_t   rx_buffer_length,
                          uint32_t src_address);

/**
 * @brief Function for writing data to QSPI memory.
 *
 * Write, read, and erase operations check memory device busy state before starting the operation.
 * If the memory is busy, the resulting action depends on the mode in which the write operation is used:
 *  - blocking mode (without handler) - a delay occurs until the last operation still runs and
 *    until operation data is still being sent.
 *  - interrupt mode (with handler) - event emission occurs after the last operation
 *    and sending of operation data are finished.
 * To manually control operation execution in the memory device, use @ref nrfx_qspi_mem_busy_check
 * after executing the write function.
 * Remember that an incoming event signalizes only that data was sent to the memory device and the periheral
 * before the write operation checked if memory was busy.
 *
 * @param[in] p_tx_buffer      Pointer to the writing buffer.
 * @param[in] tx_buffer_length Size of the data to write.
 * @param[in] dst_address      Address in memory to write to.
 *
 * @retval NRFX_SUCCESS            If the operation was successful (blocking mode) or operation
 *                                 was commissioned (handler mode).
 * @retval NRFX_ERROR_BUSY         If the driver currently handles other operation.
 * @retval NRFX_ERROR_INVALID_ADDR If the provided buffer is not placed in the Data RAM region.
 */
nrfx_err_t nrfx_qspi_write(void const * p_tx_buffer,
                           size_t       tx_buffer_length,
                           uint32_t     dst_address);

/**
 * @brief Function for starting erasing of one memory block - 4KB, 64KB, or the whole chip.
 *
 * Write, read, and erase operations check memory device busy state before starting the operation.
 * If the memory is busy, the resulting action depends on the mode in which the erase operation is used:
 *  - blocking mode (without handler) - a delay occurs until the last operation still runs and
 *    until operation data is still being sent.
 *  - interrupt mode (with handler) - event emission occurs after the last operation
 *    and sending of operation data are finished.
 * To manually control operation execution in the memory device, use @ref nrfx_qspi_mem_busy_check
 * after executing the erase function.
 * Remember that an incoming event signalizes only that data was sent to the memory device and the periheral
 * before the erase operation checked if memory was busy.
 *
 * @param[in] length        Size of data to erase. See @ref nrf_qspi_erase_len_t.
 * @param[in] start_address Memory address to start erasing. If chip erase is performed, address
 *                          field is ommited.
 *
 * @retval NRFX_SUCCESS    If the operation was successful (blocking mode) or operation
 *                         was commissioned (handler mode).
 * @retval NRFX_ERROR_BUSY If the driver currently handles another operation.
 */
nrfx_err_t nrfx_qspi_erase(nrf_qspi_erase_len_t length,
                           uint32_t             start_address);

/**
 * @brief Function for starting an erase operation of the whole chip.
 *
 * @retval NRFX_SUCCESS    If the operation was successful (blocking mode) or operation
 *                         was commissioned (handler mode).
 * @retval NRFX_ERROR_BUSY If the driver currently handles another operation.
 */
nrfx_err_t nrfx_qspi_chip_erase(void);

/**
 * @brief Function for getting the current driver status and status byte of memory device with
 *        testing WIP (write in progress) bit.
 *
 * @retval NRFX_SUCCESS    If the driver and memory are ready to handle a new operation.
 * @retval NRFX_ERROR_BUSY If the driver or memory currently handle another operation.
 */
nrfx_err_t nrfx_qspi_mem_busy_check(void);

/**
 * @brief Function for sending operation code, sending data, and receiving data from the memory device.
 *
 * Use this function to transfer configuration data to memory and to receive data from memory.
 * Pointers can be addresses from flash memory.
 * This function is a synchronous function and should be used only if necessary.
 *
 * @param[in]  p_config    Pointer to the structure with opcode and transfer configuration.
 * @param[in]  p_tx_buffer Pointer to the array with data to send. Can be NULL if only opcode is transmitted.
 * @param[out] p_rx_buffer Pointer to the array for data to receive. Can be NULL if there is nothing to receive.
 *
 * @retval NRFX_SUCCESS            If the operation was successful.
 * @retval NRFX_ERROR_BUSY         If the driver currently handles other operation.
 */
nrfx_err_t nrfx_qspi_cinstr_xfer(nrf_qspi_cinstr_conf_t const * p_config,
                                 void const *                   p_tx_buffer,
                                 void *                         p_rx_buffer);
/**
 * @brief Function for sending operation code and data to the memory device with simpler configuration.
 *
 * Use this function to transfer configuration data to memory and to receive data from memory.
 * This function is a synchronous function and should be used only if necessary.
 *
 * @param[in] opcode      Operation code. Sending first.
 * @param[in] length      Length of the data to send and opcode. See @ref nrf_qspi_cinstr_len_t.
 * @param[in] p_tx_buffer Pointer to input data array.
 *
 * @retval NRFX_SUCCESS            If the operation was successful.
 * @retval NRFX_ERROR_BUSY         If the driver currently handles another operation.
 */
nrfx_err_t nrfx_qspi_cinstr_quick_send(uint8_t               opcode,
                                       nrf_qspi_cinstr_len_t length,
                                       void const *          p_tx_buffer);


void nrfx_qspi_irq_handler(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_QSPI_H__
