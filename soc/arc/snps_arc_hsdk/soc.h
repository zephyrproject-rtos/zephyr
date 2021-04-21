/*
 * Copyright (c) 2019 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Board configuration macros for HS Development Kit
 *
 * This header file is used to specify and describe board-level
 * aspects for the target.
 */

#ifndef _SOC_H_
#define _SOC_H_

#include <sys/util.h>


/* ARC HS Core IRQs */
#define IRQ_TIMER0				16
#define IRQ_TIMER1				17
#define IRQ_ICI					19

#ifndef _ASMLANGUAGE


#include <sys/util.h>
#include <random/rand32.h>

/* PINMUX IO Hardware Functions */
#define HSDK_PINMUX_FUNS		8

#define HSDK_PINMUX_FUN0		0
#define HSDK_PINMUX_FUN1		1
#define HSDK_PINMUX_FUN2		2
#define HSDK_PINMUX_FUN3		3
#define HSDK_PINMUX_FUN4		4
#define HSDK_PINMUX_FUN5		5
#define HSDK_PINMUX_FUN6		6
#define HSDK_PINMUX_FUN7		7

/* PINMUX MAX SELS */
#define HSDK_PINMUX_SELS		8

#define HSDK_PINMUX_SEL0		0
#define HSDK_PINMUX_SEL1		1
#define HSDK_PINMUX_SEL2		2
#define HSDK_PINMUX_SEL3		3
#define HSDK_PINMUX_SEL4		4
#define HSDK_PINMUX_SEL5		5
#define HSDK_PINMUX_SEL6		6
#define HSDK_PINMUX_SEL7		7


#endif /* !_ASMLANGUAGE */

#endif /* _SOC_H_ */
