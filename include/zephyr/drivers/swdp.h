/*
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup swdp_interface
 * @brief Main header file for SWDP (Serial Wire Debug Port) driver API.
 */

#ifndef ZEPHYR_INCLUDE_SWDP_H_
#define ZEPHYR_INCLUDE_SWDP_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for Serial Wire Debug Port (SWDP).
 * @defgroup swdp_interface SWDP
 * @since 3.7
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @name SWD Packet Request Bits
 *
 * Bit definitions for SWD packet request fields.
 * These bits are used to construct the 8-bit request packet header sent during an SWD transaction.
 *
 * @{
 */

/** Access Port (AP) or Debug Port (DP). 1 = AP, 0 = DP */
#define SWDP_REQUEST_APnDP			BIT(0)
/** Read (1) or Write (0) operation */
#define SWDP_REQUEST_RnW			BIT(1)
/** Address bit 2 for register selection */
#define SWDP_REQUEST_A2				BIT(2)
/** Address bit 3 for register selection */
#define SWDP_REQUEST_A3				BIT(3)

/** @} */

/**
 * @name SWD Acknowledge (ACK) Response Bits
 *
 * Bit definitions for SWD acknowledge response fields.
 * These bits are used to indicate the result of an SWD transaction.
 *
 * @{
 */

/** Transaction completed successfully */
#define SWDP_ACK_OK				BIT(0)
/** Target requests to retry the transaction later */
#define SWDP_ACK_WAIT				BIT(1)
/** Target detected a fault condition */
#define SWDP_ACK_FAULT				BIT(2)

/** @} */

/** Transfer or parity error detected during transaction */
#define SWDP_TRANSFER_ERROR			BIT(3)

/**
 * @name SWDP Interface Pin Definitions
 *
 * Pin identifiers for SWDP interface control.
 * These constants define bit positions for controlling individual pins in the SWDP interface.
 *
 * @{
 */

/** Serial Wire Clock (SWCLK) pin identifier */
#define SWDP_SWCLK_PIN			0U
/** Serial Wire Data Input/Output (SWDIO) pin identifier */
#define SWDP_SWDIO_PIN			1U
/** Active-low reset (nRESET) pin identifier */
#define SWDP_nRESET_PIN			7U

/** @} */

/**
 * Serial Wire Debug Port (SWDP) driver API.
 * This is the mandatory API any Serial Wire Debug Port driver needs to expose.
 */
struct swdp_api {
	/**
	 * @brief Write count bits to SWDIO from data LSB first
	 *
	 * @param dev SWDP device
	 * @param count Number of bits to write
	 * @param data Bits to write
	 * @return 0 on success, or error code
	 */
	int (*swdp_output_sequence)(const struct device *dev,
				    uint32_t count,
				    const uint8_t *data);

	/**
	 * @brief Read count bits from SWDIO into data LSB first
	 *
	 * @param dev SWDP device
	 * @param count Number of bits to read
	 * @param data Buffer to store bits read
	 * @return 0 on success, or error code
	 */
	int (*swdp_input_sequence)(const struct device *dev,
				   uint32_t count,
				   uint8_t *data);

	/**
	 * @brief Perform SWDP transfer and store response
	 *
	 * @param dev SWDP device
	 * @param request SWDP request bits
	 * @param data Data to be transferred with request
	 * @param idle_cycles Idle cycles between request and response
	 * @param response Buffer to store response (ACK/WAIT/FAULT)
	 * @return 0 on success, or error code
	 */
	int (*swdp_transfer)(const struct device *dev,
			     uint8_t request,
			     uint32_t *data,
			     uint8_t idle_cycles,
			     uint8_t *response);

	/**
	 * @brief Set SWCLK, SWDPIO, and nRESET pins state
	 * @note The bit positions are defined by the SWDP_*_PIN macros.
	 *
	 * @param dev SWDP device
	 * @param pins Bitmask of pins to set
	 * @param value Value to set pins to
	 * @return 0 on success, or error code
	 */
	int (*swdp_set_pins)(const struct device *dev,
			     uint8_t pins, uint8_t value);

	/**
	 * @brief Get SWCLK, SWDPIO, and nRESET pins state
	 * @note The bit positions are defined by the SWDP_*_PIN macros.
	 *
	 * @param dev SWDP device
	 * @param state Place to store pins state
	 * @return 0 on success, or error code
	 */
	int (*swdp_get_pins)(const struct device *dev, uint8_t *state);

	/**
	 * @brief Set SWDP clock frequency
	 *
	 * @param dev SWDP device
	 * @param clock Clock frequency in Hz
	 * @return 0 on success, or error code
	 */
	int (*swdp_set_clock)(const struct device *dev, uint32_t clock);

	/**
	 * @brief Configure SWDP interface
	 *
	 * @param dev SWDP device
	 * @param turnaround Line turnaround cycles
	 * @param data_phase Always generate Data Phase (also on WAIT/FAULT)
	 * @return 0 on success, or error code
	 */
	int (*swdp_configure)(const struct device *dev,
			      uint8_t turnaround,
			      bool data_phase);

	/**
	 * @brief Enable interface, set pins to default state
	 *
	 * @note SWDPIO is set to output mode, SWCLK and nRESET are set to high level.
	 *
	 * @param dev SWDP device
	 * @return 0 on success, or error code
	 */
	int (*swdp_port_on)(const struct device *dev);

	/**
	 * @brief Disable interface, set pins to High-Z mode
	 *
	 * @param dev SWDP device
	 * @return 0 on success, or error code
	 */
	int (*swdp_port_off)(const struct device *dev);
};

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_SWDP_H_ */
