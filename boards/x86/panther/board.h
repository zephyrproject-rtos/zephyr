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


#if defined(CONFIG_WIFI_WINC1500)

/* GPIO numbers where the WINC1500 module is connected to */
#define CONFIG_WINC1500_GPIO_CHIP_EN 6 /* AP_GPIO6_ADC6 EXTERNAL_PAD_6 Out    */
#define CONFIG_WINC1500_GPIO_WAKE    5 /* AP_GPIO5_ADC5 EXTERNAL_PAD_5 Out    */
#define CONFIG_WINC1500_GPIO_IRQN    4 /* AP_GPIO4_ADC4 EXTERNAL_PAD_4 In Irq */
#define CONFIG_WINC1500_GPIO_RESET_N 0 /* AP_GPIO_AON0  AON_GPIO_PAD_0 Out    */

enum winc1500_gpio_index {
	/* If all the GPIOs can be served by same driver, then you
	 * can set the values to be the same. The first enum should
	 * always have a value of 0.
	 */
	WINC1500_GPIO_IDX_CHIP_EN = 0,
	WINC1500_GPIO_IDX_WAKE    = 0,
	WINC1500_GPIO_IDX_IRQN    = 0,

	WINC1500_GPIO_IDX_RESET_N = 1,

	WINC1500_GPIO_IDX_LAST_ENTRY
};

struct device **winc1500_configure_gpios(void);
void winc1500_configure_intgpios(void);

#endif /* CONFIG_WIFI_WINC1500 */

#endif /* __BOARD_H__ */
