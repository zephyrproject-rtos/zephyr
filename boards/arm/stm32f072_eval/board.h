/*
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* TAMPER push button */
#define TAMPER_PB_GPIO_PORT	"GPIOC"
#define TAMPER_PB_GPIO_PIN	13

/* Joystick Right push button */
#define JOYSTICK_RIGHT_PB_GPIO_PORT	"GPIOE"
#define JOYSTICK_RIGHT_PB_GPIO_PIN	3

/* Joystick Left push button */
#define JOYSTICK_LEFT_PB_GPIO_PORT	"GPIOF"
#define JOYSTICK_LEFT_PB_GPIO_PIN	2

/* Joystick UP push button */
#define JOYSTICK_UP_PB_GPIO_PORT	"GPIOF"
#define JOYSTICK_UP_PB_GPIO_PIN		9

/* Joystick Down push button */
#define JOYSTICK_DOWN_PB_GPIO_PORT	"GPIOF"
#define JOYSTICK_DOWN_PB_GPIO_PIN	10

/* Joystick Sel push button */
#define JOYSTICK_SEL_PB_GPIO_PORT	"GPIOA"
#define JOYSTICK_SEL_PB_GPIO_PIN	0

/* LD1 green LED */
#define LD1_GPIO_PORT	"GPIOD"
#define LD1_GPIO_PIN	8

/* LD2 orange LED */
#define LD2_GPIO_PORT	"GPIOD"
#define LD2_GPIO_PIN	9

/* LD3 red LED */
#define LD3_GPIO_PORT	"GPIOD"
#define LD3_GPIO_PIN	10

/* LD4 blue LED */
#define LD4_GPIO_PORT	"GPIOD"
#define LD4_GPIO_PIN	11

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME	TAMPER_PB_GPIO_PORT
#define SW0_GPIO_PIN	TAMPER_PB_GPIO_PIN
#define LED0_GPIO_PORT	LD1_GPIO_PORT
#define LED0_GPIO_PIN	LD1_GPIO_PIN
#define LED1_GPIO_PORT	LD2_GPIO_PORT
#define LED1_GPIO_PIN	LD2_GPIO_PIN

#endif /* __INC_BOARD_H */
