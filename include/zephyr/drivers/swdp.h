/*
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Serial Wire Debug Port interface driver API
 */

#ifndef ZEPHYR_INCLUDE_SWDP_H_
#define ZEPHYR_INCLUDE_SWDP_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SWDP packet request bits */
#define SWDP_REQUEST_APnDP			BIT(0)
#define SWDP_REQUEST_RnW			BIT(1)
#define SWDP_REQUEST_A2				BIT(2)
#define SWDP_REQUEST_A3				BIT(3)

/* SWDP acknowledge response bits */
#define SWDP_ACK_OK				BIT(0)
#define SWDP_ACK_WAIT				BIT(1)
#define SWDP_ACK_FAULT				BIT(2)

/* SWDP transfer or parity error */
#define SWDP_TRANSFER_ERROR			BIT(3)

/* SWDP Interface pins */
#define SWDP_SWCLK_PIN			0U
#define SWDP_SWDIO_PIN			1U
#define SWDP_nRESET_PIN			7U

/*
 * Serial Wire Interface (SWDP) driver API.
 * This is the mandatory API any Serial Wire driver needs to expose.
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

#endif /* ZEPHYR_INCLUDE_SWDP_H_ */
