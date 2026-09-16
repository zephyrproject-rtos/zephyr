/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NCV7430_MOCK_H_
#define NCV7430_MOCK_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

/* NCV7430 supported writing frame identifiers */
#define NCV7430_FRAME_ID_SET_LED_CONTROL_WRITING 0x23
#define NCV7430_FRAME_ID_SET_LED_COLOR_WRITING   0x24

/* Standard LIN frame identifiers */
#define NCV7430_COMMANDER_COMMAND  0x3C
#define NCV7430_RESPONDER_RESPONSE 0x3D

/* NCV7430 data lengths */
#define NCV7430_8_BYTES 8
#define NCV7430_4_BYTES 4

/* For filling unused payload bytes. Unused bytes must be set as 1 for NCV7430 */
#define NCV7430_FILL 0xff

/* NCV7430 LIN node address */
#define NCV7430_NODE_ADDRESS 0x00

/* NCV7430 LED enable mask definitions */
#define NCV7430_LED_R_EN_MSK BIT(0)
#define NCV7430_LED_G_EN_MSK BIT(1)
#define NCV7430_LED_B_EN_MSK BIT(2)

/* RGB LED color structure */
struct rgb_led_color {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

/**
 * @brief Initialize the NCV7430 mock responder state and register its LIN event callback.
 *
 * @param dev Pointer to the LIN device structure, configured in LIN_MODE_RESPONDER.
 * @param addr NCV7430 node address to answer to.
 * @return 0 on success, negative errno code on failure.
 */
int ncv7430_mock_init(const struct device *dev, uint8_t addr);

#endif /* NCV7430_MOCK_H_ */
