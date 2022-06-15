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

#include <zephyr/sys/util.h>


/* ARC HS Core IRQs */
#define IRQ_TIMER0				16
#define IRQ_TIMER1				17
#define IRQ_ICI					19

#ifndef _ASMLANGUAGE


#include <zephyr/sys/util.h>
#include <zephyr/random/rand32.h>

#endif /* !_ASMLANGUAGE */

#endif /* _SOC_H_ */
