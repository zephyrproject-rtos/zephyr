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
	/* Generate SWJ Sequence according to sequence bit count and bit data */
	int (*swdp_output_sequence)(const struct device *dev,
				    uint32_t count,
				    const uint8_t *data);

	/* Read count bits from SWDIO into data LSB first */
	int (*swdp_input_sequence)(const struct device *dev,
				   uint32_t count,
				   uint8_t *data);

	/*
	 * Perform SWDP transfer based on host request value and store
	 * acknowledge response bits ACK[0:2].
	 */
	int (*swdp_transfer)(const struct device *dev,
			     uint8_t request,
			     uint32_t *data,
			     uint8_t idle_cycles,
			     uint8_t *response);

	/* Set SWCLK, SWDPIO, and nRESET pins state */
	int (*swdp_set_pins)(const struct device *dev,
			     uint8_t pins, uint8_t value);

	/* Get SWCLK, SWDPIO, and nRESET pins state */
	int (*swdp_get_pins)(const struct device *dev, uint8_t *state);

	/* Set SWCLK frequency */
	int (*swdp_set_clock)(const struct device *dev, uint32_t clock);

	/*
	 * Configure interface, line turnaround and whether data phase is
	 * forced after WAIN and FAULT response.
	 */
	int (*swdp_configure)(const struct device *dev,
			      uint8_t turnaround,
			      bool data_phase);

	/*
	 * Enable interface, set SWDPIO to output mode
	 * and set SWCLK and nRESET to default high level.
	 */
	int (*swdp_port_on)(const struct device *dev);

	/* Disables interface, set SWCLK, SWDPIO, nRESET to High-Z mode. */
	int (*swdp_port_off)(const struct device *dev);
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SWDP_H_ */
