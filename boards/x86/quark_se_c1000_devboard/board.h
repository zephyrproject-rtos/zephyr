/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#if defined(CONFIG_NETWORKING_WITH_15_4_TI_CC2520)

/* GPIO numbers where the TI cc2520 chip is connected to */
#define CONFIG_CC2520_GPIO_VREG_EN	0  /* PIN ?, ATP_AON_INT0 (out) */
#define CONFIG_CC2520_GPIO_RESET	1  /* PIN ?, ATP_AON_INT1 (out) */
#define CONFIG_CC2520_GPIO_FIFO		4  /* PIN 4, GPIO4 (in) */
#define CONFIG_CC2520_GPIO_FIFOP	5  /* PIN 5, GPIO5 (in) */
#define CONFIG_CC2520_GPIO_CCA		6  /* PIN 6, GPIO6 (in) */
#define CONFIG_CC2520_GPIO_SFD		29 /* PIN 33, GPIO29 (in) */

enum cc2520_gpio_index {
	/* If all the GPIOs can be served by same driver, then you
	 * can set the values to be the same. The first enum should
	 * always have a value of 0.
	 */
	CC2520_GPIO_IDX_RESET	= 0,
	CC2520_GPIO_IDX_VREG_EN	= 0,
	CC2520_GPIO_IDX_SFD	= 1,
	CC2520_GPIO_IDX_CCA	= 1,
	CC2520_GPIO_IDX_FIFOP	= 1,
	CC2520_GPIO_IDX_FIFO	= 1,

	CC2520_GPIO_IDX_LAST_ENTRY
};

#endif /* CONFIG_NETWORKING_WITH_15_4_TI_CC2520 */

#if defined(CONFIG_USB)
/* GPIO driver name */
#define USB_GPIO_DRV_NAME	CONFIG_GPIO_QMSI_0_NAME
/* GPIO pin for enabling VBUS */
#define USB_VUSB_EN_GPIO	28
#endif

#endif /* __BOARD_H__ */
