/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <soc.h>

/* Green LED */
#define LED_G_GPIO_PIN   25

/* Yellow LED */
#define LED_Y_GPIO_PIN   26

/* Onboard LED */
#define LED0_GPIO_PORT  CONFIG_GPIO_QMSI_0_NAME
#define LED0_GPIO_PIN   LED_G_GPIO_PIN


/* AON5 */
#define UART_CONSOLE_SWITCH         5

#if defined(CONFIG_USB)
/* GPIO driver name */
#define USB_GPIO_DRV_NAME       CONFIG_GPIO_QMSI_0_NAME
/* GPIO pin for enabling VBUS */
#define USB_VUSB_EN_GPIO        28
#endif

#endif /* __BOARD_H__ */
