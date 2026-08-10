/*
 * Copyright 2021-2023,2025,2026 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __UWB_BUS_INTERFACE_H__
#define __UWB_BUS_INTERFACE_H__

#ifdef UWBIOT_USE_FTR_FILE
#include "uwb_iot_ftr.h"
#else
#include "uwb_iot_ftr_default.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <uwb_bus_board.h>
#include <uwb_uwbs_tml_io.h>

/** Max Time to wait for SPI data transfer completion */
#define MAX_UWBS_SPI_TRANSFER_TIMEOUT (1000)

/** @defgroup uwb_bus_status UWB BUS Interface Status Codes
 *
 * @{
 */

/* doc:start: Status codes for UWB bus interface */

/**
 * @brief Status code for HOST
 *
 */
typedef enum {
	/** Everything went well */
	kUWB_bus_Status_OK = 0,
	/** There was a failure */
	kUWB_bus_Status_FAILED,
	/** There was a ADDR NAK */
	kUWB_bus_Status_Busy,
	kUWB_bus_Status_Idle,
	kUWB_bus_Status_Nak,
	kUWB_bus_Status_ArbitrationLost,
	kUWB_bus_Status_Timeout,
	kUWB_bus_Status_Addr_Nak,
} uwb_bus_status_t;

/** @} */
/* doc:end: Status codes for UWB bus interface */

/** @defgroup uwb_bus UWB BUS Interface
 *
 * This layer depends on ``uwb_bus_board.h``, which is board specific.
 *
 * ``uwb_bus_board.h`` must implement / define the following
 *
 * - structure ``uwb_bus_board_ctx_t``
 *
 * The implementation of all these APIs would be board specific and
 * tightly coupled to the board.
 *
 * @{
 */

/* doc:start: UWB Bus (SPI) Interface */

/**
 * @brief Initailize bus interface
 *
 * When this API is called
 *
 * - All requried semaphores and mutexes are created
 * - bus interface (SPI) is initialised.
 *
 * @param      pCtx  The context
 */
uwb_bus_status_t uwb_bus_init(uwb_bus_board_ctx_t *pCtx);

/**
 * @brief De-Iniailize the bus interface and free up references and context
 *
 * When this API is called
 *
 * - All requried semaphores and mutexes are destryoed
 * - bus interface (SPI) is de-Iniailized.
 *
 * @param      pCtx  The context
 */
uwb_bus_status_t uwb_bus_deinit(uwb_bus_board_ctx_t *pCtx);

/**
 * @brief Reset UWB BUS
 * @param      pCtx  The context
 */
uwb_bus_status_t uwb_bus_reset(uwb_bus_board_ctx_t *pCtx);

/** @} */

/** @defgroup uwb_bus_data UWB BUS DATA Interface
 *
 * @{
 */

/**
 * @brief Transmit a data frame
 *
 * @param      pCtx    The context
 * @param[in]  pBuf    The data that we want to transmit
 * @param[in]  bufLen  The data length
 *
 * @retval kUWB_bus_Status_OK
 * @retval kUWB_bus_Status_FAILED
 */
uwb_bus_status_t uwb_bus_data_tx(uwb_bus_board_ctx_t *pCtx, uint8_t *pBuf, size_t bufLen);

/**
 * @brief Receive a data frame
 *
 * @param      pCtx     The context
 * @param[out] pBuf     The pointer where we copy received data
 * @param[in]  pBufLen  Input: No of bytes to read.
 *
 * @retval kUWB_bus_Status_OK
 * @retval kUWB_bus_Status_FAILED
 */
uwb_bus_status_t uwb_bus_data_rx(uwb_bus_board_ctx_t *pCtx, uint8_t *pBuf, size_t pBufLen);

/* doc:end: UWB Bus (SPI) Interface */

#if (UWBIOT_UWBD_SR04X)
/**
 * @brief Transmits a data frame over SPI without asserting the Chip Select (CS) line.
 *
 * This function performs an SPI data transfer without asserting the Chip Select (CS)
 * line, which is typically controlled via configuration flags. When using this API,
 * the application is responsible for managing the CS line externally (e.g., manually
 * asserting and deasserting CS before and after the transfer).
 *
 * @param[in]  pCtx     Pointer to the board context.
 * @param[in]  pBuf     Pointer to the data buffer to be transmitted.
 * @param[in]  bufLen   Length of the data buffer in bytes.
 *
 * @retval kUWB_bus_Status_OK       Data transmitted successfully.
 * @retval kUWB_bus_Status_FAILED   Transmission failed.
 */
uwb_bus_status_t uwb_bus_data_tx_no_assert(uwb_bus_board_ctx_t *pCtx, uint8_t *pBuf, size_t bufLen);

/** @} */

/** @defgroup uwb_bus_data_crc  UWB BUS DATA CRC modem apis
 *
 * @{
 */

/**
 * @brief Computes the CRC16 checksum of the input data using the XMODEM algorithm.
 *
 * This function calculates a 16-bit CRC over the input buffer using the CRC16-XMODEM algorithm.
 * The computed CRC value is stored in the board context.
 *
 * Note: This function only computes the CRC. It does not perform CRC verification
 * against an expected value. If CRC validation is required, the application must
 * compare the computed result (`pCtx->crc`) with the expected CRC separately.
 *
 * @param[in]  pCtx    Pointer to the board context. Must not be NULL.
 * @param[in]  input   Pointer to the input data buffer.
 * @param[in]  len     Length of the input data in bytes.
 *
 * @retval kUWB_bus_Status_OK       CRC computation completed successfully.
 * @retval kUWB_bus_Status_FAILED   CRC computation failed due to invalid context.
 */
uwb_bus_status_t uwb_bus_data_crc16_xmodem(uwb_bus_board_ctx_t *pCtx, uint8_t *input, size_t len);

/** @} */

/** @defgroup uwb_bus_reset reset SR040 apis
 *
 * @{
 */

/**
 * @brief Asserts the reset signal for the SR040 module.
 *
 * This function drives the RSTN GPIO pin low to initiate a hardware reset
 * of the SR040 module. It is typically used to bring the module into
 * a known state before initialization or recovery from error conditions.
 *
 */
void uwb_bus_reset_sr040_assert();

/**
 * @brief      Wait for UWBS RDYn
 *
 * This is used while writing the data to UWBS
 *
 * @param      pCtx        The context
 * @param[in]  timeout_ms  The timeout milliseconds
 *
 * @retval kUWB_bus_Status_OK IRQ was triggered before the timeout
 * @retval kUWB_bus_Status_FAILED IRQ was not triggered, and we timed out.
 *
 */
uwb_bus_status_t uwb_bus_io_rdy_wait(uwb_bus_board_ctx_t *pCtx, uint32_t timeout_ms);
/** @} */
#endif // UWBIOT_UWBD_SR04X

/** @defgroup uwb_bus_io UWB BUS IO Management
 *
 * See @ref uwb_uwbs_io
 *
 * @{
 */

/* doc:start: UWB Bus IO Interface */

/**
 * @brief      Initialise GPIO
 *
 * When this API is called
 *
 * - All IO Pins are set to their respective states
 *
 * @param      pCtx  The context
 */
uwb_bus_status_t uwb_bus_io_init(uwb_bus_board_ctx_t *pCtx);

/**
 * @brief      De Initialise GPIO's
 *
 * @param      pCtx  The context
 */
uwb_bus_status_t uwb_bus_io_deinit(uwb_bus_board_ctx_t *pCtx);

/**
 * @brief  Set value of a GPIO
 *
 * @param      pCtx  The context
 * @param      gpioPin  pin
 * @param      gpioValue  value
 */
uwb_bus_status_t uwb_bus_io_val_set(uwb_bus_board_ctx_t *pCtx, uwbs_io_t gpioPin,
				    uwbs_io_state_t gpioValue);

/**
 * @brief Callback called when IRQ is triggered
 *
 * IRQ Mapping APIs are "Vendor/Platform SDK" specific.  And the SDK specific IRQ Callback
 * implementation must call this API with right context.
 *
 * @param      pCtx  The context
 */
void uwb_bus_io_irq_cb(void *pCtx);

/**
 * @brief Enables the UWBS IRQ with a board-specific callback function.
 *
 * This function activates the IRQ for the UWBS interface
 * and registers a board-specific callback handler to manage the interrupt.
 *
 * @param[in]  pCtx   Pointer to the board context.
 *
 * @retval kUWB_bus_Status_OK       IRQ successfully enabled.
 * @retval kUWB_bus_Status_FAILED   Failed to enable IRQ.
 */
uwb_bus_status_t uwb_bus_io_uwbs_irq_enable(uwb_bus_board_ctx_t *pCtx);

/**
 * @brief Disables the UWBS IRQ with a board-specific callback function.
 *
 * This function deactivates the IRQ for the UWBS interface
 * and unregisters the associated board-specific callback handler.
 *
 * @param[in]  pCtx   Pointer to the board context.
 *
 * @retval kUWB_bus_Status_OK       IRQ successfully disabled.
 * @retval kUWB_bus_Status_FAILED   Failed to disable IRQ.
 */
uwb_bus_status_t uwb_bus_io_uwbs_irq_disable(uwb_bus_board_ctx_t *pCtx);

#if (UWBIOT_TML_I2C || UWBIOT_TML_SPI)
/**
 * @brief      Get value of a GPIO
 *
 * @param      pCtx  The context
 * @param      gpioPin  pin
 * @param      pGpioValue  value
 */
uwb_bus_status_t uwb_bus_io_val_get(uwb_bus_board_ctx_t *pCtx, uwbs_io_t gpioPin,
				    uwbs_io_state_t *pGpioValue);

/**
 * @brief      Wait for UWBS IRQ
 *
 * This is used while reading the data from UWBS
 *
 * @param      pCtx        The context
 * @param[in]  timeout_ms  The timeout milliseconds
 *
 * @retval kUWB_bus_Status_OK IRQ was triggered before the timeout
 * @retval kUWB_bus_Status_FAILED IRQ was not triggered, and we timed out.
 *
 */
uwb_bus_status_t uwb_bus_io_irq_wait(uwb_bus_board_ctx_t *pCtx, uint32_t timeout_ms);

/**
 * @brief Enables the specified IRQ and registers a callback handler.
 *
 * This function enables an interrupt request (IRQ) on the specified pin and
 * associates it with a user-defined callback function to handle the interrupt.
 *
 * @param[in]  pCtx        Pointer to the board context.
 * @param[in]  irqPin      The IRQ pin to be enabled.
 * @param[in]  pCallback   Pointer to the interrupt handler callback function.
 *
 * @retval kUWB_bus_Status_OK       IRQ successfully enabled and callback registered.
 * @retval kUWB_bus_Status_FAILED   Failed to enable IRQ or register callback.
 */
uwb_bus_status_t uwb_bus_io_irq_en(uwb_bus_board_ctx_t *pCtx, uwbs_io_t irqPin,
				   uwbs_io_callback *pCallback);

/**
 * @brief Disables the specified IRQ.
 *
 * This function disables the interrupt request (IRQ) on the specified pin.
 *
 * @param[in]  pCtx     Pointer to the board context.
 * @param[in]  irqPin   The IRQ pin to be disabled.
 *
 * @retval kUWB_bus_Status_OK       IRQ successfully disabled.
 * @retval kUWB_bus_Status_FAILED   Failed to disable IRQ.
 */
uwb_bus_status_t uwb_bus_io_irq_dis(uwb_bus_board_ctx_t *pCtx, uwbs_io_t irqPin);

#endif // UWBIOT_TML_I2C || UWBIOT_TML_SPI

/* doc:end: UWB Bus IO Interface */

/** @} */

/** @defgroup nfc_wrapper NFC wrapper functions used by wrbl-nfc-sdk
 * @{
 */

/* doc:start: NFC Wrapper Interface */

/**
 * @brief Sets the NFC Enable (VEN) GPIO pin to the specified value.
 *
 * This is a wrapper function for the wrbl-nfc-sdk. Internally, it calls
 * `uwb_bus_io_val_set` using the board context to set the VEN pin.
 *
 * @param[in] gpioValue  Value to set on the VEN pin (typically 0 or 1).
 */
void nfc_ven_val_set(uint8_t gpioValue);

/**
 * @brief Gets the current value of the NFC Enable (VEN) GPIO pin.
 *
 * This is a wrapper function for the wrbl-nfc-sdk. Internally, it calls
 * `uwb_bus_io_val_get` using the board context to read the VEN pin value.
 *
 * @return uint32_t      Current value of the VEN pin (typically 0 or 1).
 */
uint32_t nfc_ven_val_get();

/**
 * @brief Gets the current value of the NFC IRQ GPIO pin.
 *
 * This is a wrapper function for the wrbl-nfc-sdk. Internally, it calls
 * `uwb_bus_io_val_get` using the board context to read the IRQ pin value.
 *
 * @return uint32_t      Current value of the IRQ pin (typically 0 or 1).
 */
uint32_t nfc_irq_val_get();

/* doc:end: NFC Wrapper Interface */

/** @} */

/** @defgroup uwb_bus_i2c I2C bus interface functions
 * @{
 */

/* doc:start: UWB BUS I2C Interface Implementation */

/**
 * @brief Initializes the I2C bus interface.
 *
 * This API registers and returns the I2C bus context pointer required for
 * subsequent I2C operations.
 *
 * @return void*  Pointer to the initialized I2C bus context.
 */
void *uwb_bus_i2c_init(void);

/**
 * @brief Deinitializes the I2C bus interface.
 *
 * This API releases resources associated with the I2C bus context.
 *
 * @param[in] context  Pointer to the registered I2C bus context.
 */
void uwb_bus_i2c_deinit(void *const context);

/**
 * @brief Transmits a data frame over I2C.
 *
 * This API sends a data buffer to the specified I2C slave address using the
 * provided I2C bus context.
 *
 * @param[in] context   Pointer to the registered I2C bus context.
 * @param[in] address   I2C slave address.
 * @param[in] txData    Pointer to the data buffer to transmit.
 * @param[in] txLen     Length of the data buffer in bytes.
 *
 * @retval kUWB_bus_Status_OK       Data transmitted successfully.
 * @retval kUWB_bus_Status_FAILED   Transmission failed.
 */
uwb_bus_status_t uwb_bus_i2c_data_tx(const void *const context, const uint8_t address,
				     const uint8_t *const txData, const uint32_t txLen);

/**
 * @brief Receives a data frame over I2C.
 *
 * This API reads a data buffer from the specified I2C slave address using the
 * provided I2C bus context.
 *
 * @param[in]  context   Pointer to the registered I2C bus context.
 * @param[in]  address   I2C slave address.
 * @param[out] rxData    Pointer to the buffer to store received data.
 * @param[in]  rxLen     Number of bytes read.
 *
 * @retval kUWB_bus_Status_OK       Data received successfully.
 * @retval kUWB_bus_Status_FAILED   Reception failed.
 */
uwb_bus_status_t uwb_bus_i2c_data_rx(const void *const context, const uint8_t address,
				     uint8_t *const rxData, const uint32_t rxLen);

/* doc:end: UWB BUS I2C Interface Implementation */

/** @} */

#endif
