/*
 * Copyright (c) 2018 Christian Taedcke
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <zephyr/devicetree.h>

/* These pins is used as part of the 9116 initialization routine */
#define RS9116_GPIO_NAME  "gpio@4000a000" /* GPIO_A */
#define RS9116_POC_GPIO_PIN   8
#define RS9116_RST_GPIO_PIN   9

/* These pins are used as part of the PAM8904E initialization routine */
#define BUZZER_ENx_GPIO_NAME  "gpio@4000a060" /* GPIO_C */
#define BUZZER_EN1_GPIO_PIN   4
#define BUZZER_EN2_GPIO_PIN   5

/* These pins are used as part of the PWM LED initialization routine */
#define WHITE_LED_GPIO_PIN    7
#define RED_LED_GPIO_PIN      8
#define GREEN_LED_GPIO_PIN    9
#define BLUE_LED_GPIO_PIN     10

#define GNSS_GPIO_NAME "gpio@4000a0f0" /* GPIO_F */
#define GNSS_BOOT_REQ 6
#define GNSS_RESET 8
#define GNSS_PWR_ON 10

#define BQ_CHIP_ENABLE 13

#if DT_NODE_EXISTS(DT_NODELABEL(pwm1))
#define LED_PWM_DEV DEVICE_DT_GET(DT_NODELABEL(pwm1))
#else
#define LED_PWM_DEV NULL
#endif

#define LED_PWM_WHITE 0
#define LED_PWM_RED   1
#define LED_PWM_GREEN 2
#define LED_PWM_BLUE  3

#define HWID_APORT		   adcPosSelAPORT4XCH23
#define VBAT_APORT		   adcPosSelAPORT3XCH10

#define VBAT_EN_PORT       gpioPortK
#define VBAT_EN_PIN        0

#endif /* __INC_BOARD_H */
