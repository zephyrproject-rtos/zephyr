/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <soc.h>

#if defined(CONFIG_IEEE802154_CC2520)

/* GPIO numbers where the TI cc2520 chip is connected to */
#define CC2520_GPIO_VREG_EN	0  /* PIN ?, ATP_AON_INT0 (out) */
#define CC2520_GPIO_RESET	1  /* PIN ?, ATP_AON_INT1 (out) */
#define CC2520_GPIO_FIFO	4  /* PIN 4, GPIO4 (in) */
#define CC2520_GPIO_FIFOP	5  /* PIN 5, GPIO5 (in) */
#define CC2520_GPIO_CCA		6  /* PIN 6, GPIO6 (in) */
#define CC2520_GPIO_SFD		29 /* PIN 33, GPIO29 (in) */

#endif /* CONFIG_IEEE802154_CC2520 */

#if defined(CONFIG_IEEE802154_CC1200)

/* GPIO numbers where the TI cc1200 chip is connected to */
#define CC1200_GPIO_GPIO0		18  /* GPIO18 (in) */

#endif /* CONFIG_IEEE802154_CC1200 */

#if defined(CONFIG_USB)
/* GPIO driver name */
#define USB_GPIO_DRV_NAME	CONFIG_GPIO_QMSI_0_NAME
/* GPIO pin for enabling VBUS */
#define USB_VUSB_EN_GPIO	28
#endif

#endif /* __BOARD_H__ */
