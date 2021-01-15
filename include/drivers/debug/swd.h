/*
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public Serial Wire Interface API
 */

#ifndef ZEPHYR_INCLUDE_SWD_H_
#define ZEPHYR_INCLUDE_SWD_H_

/**
 * @brief SWD Interface
 * @note Experimental API
 * @defgroup swd_interface SWD Interface
 * @ingroup swd_interfaces
 * @{
 */


#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup swd
 * @{
 */

/* SWD packet request bits */
#define SWD_REQUEST_APnDP			BIT(0)
#define SWD_REQUEST_RnW				BIT(1)
#define SWD_REQUEST_A2				BIT(2)
#define SWD_REQUEST_A3				BIT(3)

/* SWD acknowledge response bits */
#define SWD_ACK_OK				BIT(0)
#define SWD_ACK_WAIT				BIT(1)
#define SWD_ACK_FAULT				BIT(2)

/* SWD transfer or parity error */
#define SWD_TRANSFER_ERROR			BIT(3)

/* SWD Interface Pins */
#define DAP_SW_SWCLK_PIN			0U
#define DAP_SW_SWDIO_PIN			1U
#define DAP_SW_nRESET_PIN			7U

/**
 * @brief Generate SWJ Sequence
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param count Sequence bit count.
 * @param data Pointer to sequence bit data.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
typedef int (*swd_sequence)(const struct device *dev,
			    uint32_t count,
			    const uint8_t *data);

/**
 * @brief SWD Transfer
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param request 8-bit host request
 * @param data R/W DATA[31:0]
 * @param response acknowledge response ACK[0:2]
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
typedef int (*swd_transfer)(const struct device *dev,
			    uint8_t request,
			    uint32_t *data,
			    uint8_t idle_cycles,
			    uint8_t *response);

/**
 * @brief Set SWCLK, SWDIO, and nRESET pins state.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Pins to write.
 * @param value Condition that pins should take.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
typedef int (*swd_set_pins)(const struct device *dev,
			    uint8_t pins, uint8_t value);

/**
 * @brief Get SWCLK, SWDIO, and nRESET pins state.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pins Pins to read.
 * @param state Pins state.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
typedef int (*swd_get_pins)(const struct device *dev, uint8_t *state);

/**
 * @brief Set SWCLK frequency
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param clock Frequency of SWCLK.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
typedef int (*swd_set_clock)(const struct device *dev, uint32_t clock);

/**
 * @brief Set or get SWCLK, SWDIO, and nRESET pins state.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param turnaround Line turnaround.
 * @param data_phase Force Data Phase after WAIT or FAULT response.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
typedef int (*swd_configure)(const struct device *dev,
			     uint8_t turnaround,
			     bool data_phase);

/**
 * @brief Setup SWD I/O pins: SWCLK, SWDIO, and nRESET.
 * Configures the DAP Hardware I/O pins for Serial Wire Debug (SWD) mode:
 * SWCLK, SWDIO, nRESET to output mode and set to default high level.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
typedef int (*swd_port_on)(const struct device *dev);

/**
 * @brief  Disable SWD I/O Pins.
 *
 * Disables the DAP Hardware I/O pins which configures:
 * SWCLK, SWDIO, nRESET to High-Z mode.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
typedef int (*swd_port_off)(const struct device *dev);

/**
 * @brief Serial Wire Interface (SWD) driver API
 * This is the mandatory API any Serial Wire driver needs to expose.
 */
struct swd_api {
	swd_sequence sw_sequence;
	swd_transfer sw_transfer;
	swd_set_pins sw_set_pins;
	swd_get_pins sw_get_pins;
	swd_set_clock sw_set_clock;
	swd_configure sw_configure;
	swd_port_on sw_port_on;
	swd_port_off sw_port_off;
};


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_SWD_H_ */

