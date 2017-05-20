/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* Red LED */
#define RED_GPIO_NAME		CONFIG_GPIO_MCUX_PORTC_NAME
#define RED_GPIO_PIN		8

/* Green LED */
#define GREEN_GPIO_NAME		CONFIG_GPIO_MCUX_PORTD_NAME
#define GREEN_GPIO_PIN		0

/* Blue LED */
#define BLUE_GPIO_NAME		CONFIG_GPIO_MCUX_PORTC_NAME
#define BLUE_GPIO_PIN		9

/* LED0. There is no physical LED on the board with this name, so create an
 * alias to the green part of the RGB LED to make the basic blinky sample
 * work.
 */
#define LED0_GPIO_PORT	GREEN_GPIO_NAME
#define LED0_GPIO_PIN	GREEN_GPIO_PIN


#define HAPTIC_MOTOR_NAME       CONFIG_GPIO_MCUX_PORTB_NAME
#define HAPTIC_MOTOR_PIN        9

#define MAX30101_GPIO_NAME	CONFIG_GPIO_MCUX_PORTA_NAME
#define MAX30101_GPIO_PIN	29

#ifdef CONFIG_SPI_0
#define SPI_0_SCK_NAME          CONFIG_GPIO_MCUX_PORTC_NAME
#define SPI_0_SCK_PIN           5

#define SPI_0_MOSI_NAME         CONFIG_GPIO_MCUX_PORTC_NAME
#define SPI_0_MOSI_PIN          6

#define SPI_0_MISO_NAME         CONFIG_GPIO_MCUX_PORTC_NAME
#define SPI_0_MISO_PIN          7
#endif  /* SPI_0 */


#ifdef CONFIG_SPI_1
#define SPI_1_SCK_NAME          CONFIG_GPIO_MCUX_PORTD_NAME
#define SPI_1_SCK_PIN           5

#define SPI_1_MOSI_NAME         CONFIG_GPIO_MCUX_PORTD_NAME
#define SPI_1_MOSI_PIN          6

#define SPI_1_MISO_NAME         CONFIG_GPIO_MCUX_PORTD_NAME
#define SPI_1_MISO_PIN          7
#endif  /* SPI_1 */


#ifdef CONFIG_SPI_2
#define SPI_2_SCK_NAME          CONFIG_GPIO_MCUX_PORTB_NAME
#define SPI_2_SCK_PIN           21

#define SPI_2_MOSI_NAME         CONFIG_GPIO_MCUX_PORTB_NAME
#define SPI_2_MOSI_PIN          22

/* This board does not have a MISO configured for SPI 2,
 * since that device is only for the OLED screen, which has
 * no data to transfer back to the MCU
 */
#endif  /* SPI_2 */

#ifdef CONFIG_HEXIWEAR_DOCKING_STATION

/* this is either an LED or a pushbutton, depending on DIP switch LED1 */
#define CLICK1_GPIO_NAME    CONFIG_GPIO_MCUX_PORTA_NAME
#define CLICK1_GPIO_PIN     12

/* this is either an LED or a pushbutton, depending on DIP switch LED2 */
#define CLICK2_GPIO_NAME    CONFIG_GPIO_MCUX_PORTA_NAME
#define CLICK2_GPIO_PIN     13

/* this is either an LED or a pushbutton, depending on DIP switch LED3 */
#define CLICK3_GPIO_NAME    CONFIG_GPIO_MCUX_PORTA_NAME
#define CLICK3_GPIO_PIN     14


#define CLICK1_AN_NAME      CONFIG_GPIO_MCUX_PORTB_NAME
#define CLICK1_AN_PIN       2

#define CLICK1_PWM_NAME     CONFIG_GPIO_MCUX_PORTA_NAME
#define CLICK1_PWM_PIN      10

#define CLICK1_RST_NAME     CONFIG_GPIO_MCUX_PORTB_NAME
#define CLICK1_RST_PIN      11

#define CLICK1_INT_NAME     CONFIG_GPIO_MCUX_PORTB_NAME
#define CLICK1_INT_PIN      13

#define CLICK1_CS_NAME      CONFIG_GPIO_MCUX_PORTC_NAME
#define CLICK1_CS_PIN       4

#define CLICK1_RX_NAME      CONFIG_GPIO_MCUX_PORTD_NAME
#define CLICK1_RX_PIN       2

#define CLICK1_TX_NAME      CONFIG_GPIO_MCUX_PORTD_NAME
#define CLICK1_TX_PIN       3

#define CLICK1_SPI_SELECT   0

#define CLICK2_AN_NAME      CONFIG_GPIO_MCUX_PORTB_NAME
#define CLICK2_AN_PIN       3

#define CLICK2_PWM_NAME     CONFIG_GPIO_MCUX_PORTA_NAME
#define CLICK2_PWM_PIN      11

#define CLICK2_RST_NAME     CONFIG_GPIO_MCUX_PORTB_NAME
#define CLICK2_RST_PIN      19

#define CLICK2_INT_NAME     CONFIG_GPIO_MCUX_PORTB_NAME
#define CLICK2_INT_PIN      8

#define CLICK2_CS_NAME      CONFIG_GPIO_MCUX_PORTC_NAME
#define CLICK2_CS_PIN       3

#define CLICK2_RX_NAME      CONFIG_GPIO_MCUX_PORTC_NAME
#define CLICK2_RX_PIN       16

#define CLICK2_TX_NAME      CONFIG_GPIO_MCUX_PORTC_NAME
#define CLICK2_TX_PIN       17

#define CLICK2_SPI_SELECT   1

#define CLICK3_AN_NAME      CONFIG_GPIO_MCUX_PORTB_NAME
#define CLICK3_AN_PIN       6

#define CLICK3_PWM_NAME     CONFIG_GPIO_MCUX_PORTA_NAME
#define CLICK3_PWM_PIN      4

#define CLICK3_RST_NAME     CONFIG_GPIO_MCUX_PORTB_NAME
#define CLICK3_RST_PIN      10

#define CLICK3_INT_NAME     CONFIG_GPIO_MCUX_PORTB_NAME
#define CLICK3_INT_PIN      7

#define CLICK3_CS_NAME      CONFIG_GPIO_MCUX_PORTC_NAME
#define CLICK3_CS_PIN       2

#define CLICK3_RX_NAME      CONFIG_GPIO_MCUX_PORTC_NAME
#define CLICK3_RX_PIN       16

#define CLICK3_TX_NAME      CONFIG_GPIO_MCUX_PORTC_NAME
#define CLICK3_TX_PIN       17

#define CLICK3_SPI_SELECT   2


#endif /* CONFIG_HEXIWEAR_DOCKING_STATION */


#endif /* __INC_BOARD_H */
