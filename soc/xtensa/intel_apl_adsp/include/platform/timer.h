/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *         Keyon Jie <yang.jie@linux.intel.com>
 */

#ifndef __PLATFORM_TIMER_H__
#define __PLATFORM_TIMER_H__

#include <stdint.h>
#include <sof/timer.h>
#include <platform/interrupt.h>

#define TIMER_COUNT	5

/* timer numbers must use associated IRQ number */
#define TIMER0		IRQ_NUM_TIMER1
#define TIMER1		IRQ_NUM_TIMER2
#define TIMER2		IRQ_NUM_TIMER3
#define TIMER3		IRQ_EXT_TSTAMP0_LVL2(0)
#define TIMER4		IRQ_EXT_TSTAMP1_LVL2(0)

#endif
