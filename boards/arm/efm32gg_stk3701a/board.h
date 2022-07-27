/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

/* This pin is used to enable the serial port using the board controller */
#define BC_ENABLE_GPIO_NODE		DT_NODELABEL(gpioe)
#define BC_ENABLE_GPIO_PIN		1

/* Ethernet specific pins */
#ifdef CONFIG_ETH_GECKO
#define ETH_PWR_ENABLE_GPIO_NODE	DT_NODELABEL(gpioi)
#define ETH_PWR_ENABLE_GPIO_PIN		10

#define ETH_RESET_GPIO_NODE		DT_NODELABEL(gpioh)
#define ETH_RESET_GPIO_PIN		7

#define ETH_REF_CLK_GPIO_NODE		DT_NODELABEL(gpiod)
#define ETH_REF_CLK_GPIO_PIN		DT_PROP_BY_IDX(DT_INST(0, silabs_gecko_ethernet), location_rmii_refclk, 2)
#define ETH_REF_CLK_LOCATION		DT_PROP_BY_IDX(DT_INST(0, silabs_gecko_ethernet), location_rmii_refclk, 0)

#endif /* CONFIG_ETH_GECKO */

#endif /* __INC_BOARD_H */
