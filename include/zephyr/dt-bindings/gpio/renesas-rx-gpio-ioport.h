/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX GPIO I/O port devicetree bindings
 *
 * This header provides GPIO drive strength definitions for Renesas RX series
 * microcontrollers used in devicetree source files.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RX_GPIO_IOPORT_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RX_GPIO_IOPORT_H_

#include <zephyr/dt-bindings/dt-util.h>

/** @brief GPIO drive strength bit position */
#define RENESAS_GPIO_DS_POS                   (8)
/** @brief GPIO drive strength mask */
#define RENESAS_GPIO_DS_MSK                   GENMASK(RENESAS_GPIO_DS_POS + 1, RENESAS_GPIO_DS_POS)
/** @brief GPIO drive strength low */
#define RENESAS_GPIO_DS_LOW                   (0x0)
/** @brief GPIO drive strength middle */
#define RENESAS_GPIO_DS_MIDDLE                BIT(RENESAS_GPIO_DS_POS)
/** @brief GPIO drive strength high speed high drive */
#define RENESAS_GPIO_DS_HIGH_SPEED_HIGH_DRIVE BIT(RENESAS_GPIO_DS_POS + 1)
/** @brief GPIO drive strength high drive */
#define RENESAS_GPIO_DS_HIGH_DRIVE            GENMASK(RENESAS_GPIO_DS_POS + 1, RENESAS_GPIO_DS_POS)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RX_GPIO_IOPORT_H_ */
