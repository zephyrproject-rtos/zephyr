/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <soc.h>

/* Push button switch 0 */
#define SW0_GPIO_PIN	4
#define SW0_GPIO_NAME	CONFIG_GPIO_QMSI_1_NAME

/* Push button switch 1 */
#define SW1_GPIO_PIN	5
#define SW1_GPIO_NAME	CONFIG_GPIO_QMSI_0_NAME

/* Onboard LED */
#define LED0_GPIO_PORT  CONFIG_GPIO_QMSI_0_NAME
#define LED0_GPIO_PIN   25

#if defined(CONFIG_IEEE802154_CC2520_LEGACY) || \
	defined(CONFIG_IEEE802154_CC2520) || \
	defined(CONFIG_IEEE802154_CC2520_RAW)

/* GPIO numbers where the TI cc2520 chip is connected to */
#define CC2520_GPIO_VREG_EN	0  /* PIN ?, ATP_AON_INT0 (out) */
#define CC2520_GPIO_RESET	1  /* PIN ?, ATP_AON_INT1 (out) */
#define CC2520_GPIO_FIFO	4  /* PIN 4, GPIO4 (in) */
#define CC2520_GPIO_FIFOP	5  /* PIN 5, GPIO5 (in) */
#define CC2520_GPIO_CCA		6  /* PIN 6, GPIO6 (in) */
#define CC2520_GPIO_SFD		29 /* PIN 33, GPIO29 (in) */

#endif /* CONFIG_IEEE802154_CC2520_LEGACY || CONFIG_IEEE802154_CC2520 ||
	* CONFIG_IEEE802154_CC2520_RAW
	*/

#if defined(CONFIG_USB)
/* GPIO driver name */
#define USB_GPIO_DRV_NAME	CONFIG_GPIO_QMSI_0_NAME
/* GPIO pin for enabling VBUS */
#define USB_VUSB_EN_GPIO	28
#endif

#endif /* __BOARD_H__ */
