/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_TIMERS_TI_DMTIMER_H_
#define ZEPHYR_DRIVERS_TIMERS_TI_DMTIMER_H_

#include <zephyr/devicetree.h>

#define TI_DM_TIMER_TIDR           (0x00)
#define TI_DM_TIMER_TIOCP_CFG      (0x10)
#define TI_DM_TIMER_IRQ_EOI        (0x20)
#define TI_DM_TIMER_IRQSTATUS_RAW  (0x24)
#define TI_DM_TIMER_IRQSTATUS      (0x28) /* Interrupt status register */
#define TI_DM_TIMER_IRQENABLE_SET  (0x2c) /* Interrupt enable register */
#define TI_DM_TIMER_IRQENABLE_CLR  (0x30) /* Interrupt disable register */
#define TI_DM_TIMER_IRQWAKEEN      (0x34)
#define TI_DM_TIMER_TCLR           (0x38) /* Control register */
#define TI_DM_TIMER_TCRR           (0x3c) /* Counter register */
#define TI_DM_TIMER_TLDR           (0x40) /* Load register */
#define TI_DM_TIMER_TTGR           (0x44)
#define TI_DM_TIMER_TWPS           (0x48)
#define TI_DM_TIMER_TMAR           (0x4c) /* Match register */
#define TI_DM_TIMER_TCAR1          (0x50)
#define TI_DM_TIMER_TSICR          (0x54)
#define TI_DM_TIMER_TCAR2          (0x58)
#define TI_DM_TIMER_TPIR           (0x5c)
#define TI_DM_TIMER_TNIR           (0x60)
#define TI_DM_TIMER_TCVR           (0x64)
#define TI_DM_TIMER_TOCR           (0x68)
#define TI_DM_TIMER_TOWR           (0x6c)

#define TI_DM_TIMER_IRQSTATUS_MAT_IT_FLAG_SHIFT                  (0)
#define TI_DM_TIMER_IRQSTATUS_MAT_IT_FLAG_MASK                   (0x00000001)

#define TI_DM_TIMER_IRQSTATUS_OVF_IT_FLAG_SHIFT                  (1)
#define TI_DM_TIMER_IRQSTATUS_OVF_IT_FLAG_MASK                   (0x00000002)

#define TI_DM_TIMER_IRQSTATUS_TCAR_IT_FLAG_SHIFT                 (2)
#define TI_DM_TIMER_IRQSTATUS_TCAR_IT_FLAG_MASK                  (0x00000004)

#define TI_DM_TIMER_IRQENABLE_SET_MAT_EN_FLAG_SHIFT              (0)
#define TI_DM_TIMER_IRQENABLE_SET_MAT_EN_FLAG_MASK               (0x00000001)

#define TI_DM_TIMER_IRQENABLE_SET_OVF_EN_FLAG_SHIFT              (1)
#define TI_DM_TIMER_IRQENABLE_SET_OVF_EN_FLAG_MASK               (0x00000002)

#define TI_DM_TIMER_IRQENABLE_SET_TCAR_EN_FLAG_SHIFT             (2)
#define TI_DM_TIMER_IRQENABLE_SET_TCAR_EN_FLAG_MASK              (0x00000004)

#define TI_DM_TIMER_IRQENABLE_CLR_MAT_EN_FLAG_SHIFT              (0)
#define TI_DM_TIMER_IRQENABLE_CLR_MAT_EN_FLAG_MASK               (0x00000001)

#define TI_DM_TIMER_IRQENABLE_CLR_OVF_EN_FLAG_SHIFT              (1)
#define TI_DM_TIMER_IRQENABLE_CLR_OVF_EN_FLAG_MASK               (0x00000002)

#define TI_DM_TIMER_IRQENABLE_CLR_TCAR_EN_FLAG_SHIFT             (2)
#define TI_DM_TIMER_IRQENABLE_CLR_TCAR_EN_FLAG_MASK              (0x00000004)

#define TI_DM_TIMER_TCLR_ST_SHIFT                                (0)
#define TI_DM_TIMER_TCLR_ST_MASK                                 (0x00000001)

#define TI_DM_TIMER_TCLR_AR_SHIFT                                (1)
#define TI_DM_TIMER_TCLR_AR_MASK                                 (0x00000002)

#define TI_DM_TIMER_TCLR_PTV_SHIFT                               (2)
#define TI_DM_TIMER_TCLR_PTV_MASK                                (0x0000001c)

#define TI_DM_TIMER_TCLR_PRE_SHIFT                               (5)
#define TI_DM_TIMER_TCLR_PRE_MASK                                (0x00000020)

#define TI_DM_TIMER_TCLR_CE_SHIFT                                (6)
#define TI_DM_TIMER_TCLR_CE_MASK                                 (0x00000040)

#define TI_DM_TIMER_TCLR_SCPWM_SHIFT                             (7)
#define TI_DM_TIMER_TCLR_SCPWM_MASK                              (0x00000080)

#define TI_DM_TIMER_TCLR_TCM_SHIFT                               (8)
#define TI_DM_TIMER_TCLR_TCM_MASK                                (0x00000300)

#define TI_DM_TIMER_TCLR_TRG_SHIFT                               (10)
#define TI_DM_TIMER_TCLR_TRG_MASK                                (0x00000c00)

#define TI_DM_TIMER_TCLR_PT_SHIFT                                (12)
#define TI_DM_TIMER_TCLR_PT_MASK                                 (0x00001000)

#define TI_DM_TIMER_TCLR_CAPT_MODE_SHIFT                         (13)
#define TI_DM_TIMER_TCLR_CAPT_MODE_MASK                          (0x00002000)

#define TI_DM_TIMER_TCLR_GPO_CFG_SHIFT                           (14)
#define TI_DM_TIMER_TCLR_GPO_CFG_MASK                            (0x00004000)

#define TI_DM_TIMER_TCRR_TIMER_COUNTER_SHIFT                     (0)
#define TI_DM_TIMER_TCRR_TIMER_COUNTER_MASK                      (0xffffffff)

#define TI_DM_TIMER_TLDR_LOAD_VALUE_SHIFT                        (0)
#define TI_DM_TIMER_TLDR_LOAD_VALUE_MASK                         (0xffffffff)

#define TI_DM_TIMER_TMAR_COMPARE_VALUE_SHIFT                     (0)
#define TI_DM_TIMER_TMAR_COMPARE_VALUE_MASK                      (0xffffffff)

#endif /* ZEPHYR_DRIVERS_TIMERS_TI_DMTIMER_H_ */
