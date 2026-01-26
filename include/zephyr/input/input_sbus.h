/*
 * Copyright 2026 Yunjie Ye <yun_small@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for sbus input driver.
 * @ingroup sbus_interface
 */

#ifndef ZEPHYR_INCLUDE_INPUT_SBUS_H_
#define ZEPHYR_INCLUDE_INPUT_SBUS_H_

/**
 * @defgroup sbus_interface SBUS
 * @ingroup input_interface_ext
 * @brief Header for SBUS Device specific input event
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The SBUS connection event code.
 */
#define INPUT_EV_SBUS_CONNECTED 0x1001u

/**
 * @brief SBUS connection state values
 *
 * Values used with INPUT_EV_SBUS_CONNECTED event code to indicate
 * the connection state of an SBUS controller.
 */
typedef enum {
	/** SBUS controller is disconnected */
	SBUS_DISCONNECTED = 0u,
	/** SBUS controller is connected */
	SBUS_CONNECTED = 1u,
} sbus_connection_state_t;

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_SBUS_H_ */
