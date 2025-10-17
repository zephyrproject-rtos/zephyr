/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_MCTP_I2C_GPIO_COMMON_H_
#define ZEPHYR_MCTP_I2C_GPIO_COMMON_H_

/** @cond INTERNAL_HIDDEN */

/* Pseudo Register Addresses */
#define MCTP_I2C_GPIO_INVALID_ADDR         0
#define MCTP_I2C_GPIO_RX_MSG_LEN_ADDR      1
#define MCTP_I2C_GPIO_RX_MSG_ADDR          2
#define MCTP_I2C_GPIO_TX_MSG_LEN_ADDR      3
#define MCTP_I2C_GPIO_TX_MSG_ADDR          4

/* Max packet size (pseudo registers are byte sized) */
#define MCTP_I2C_GPIO_MAX_PKT_SIZE 255

/** INTERNAL_HIDDEN @endcond */

#endif /* ZEPHYR_MCTP_I2C_GPIO_COMMON_H_ */
