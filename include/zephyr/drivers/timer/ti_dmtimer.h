/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_TIMERS_TI_DMTIMER_H_
#define ZEPHYR_DRIVERS_TIMERS_TI_DMTIMER_H_

#include <zephyr/devicetree.h>

#define TI_DM_TIMER_BASE_ADDR      DT_REG_ADDR(DT_INST(0, ti_am654_dmtimer))

#define TI_DM_TIMER_IRQ_NUM        DT_IRQN(DT_INST(0, ti_am654_dmtimer))
#define TI_DM_TIMER_IRQ_PRIO       DT_IRQ(DT_INST(0, ti_am654_dmtimer), priority)
#define TI_DM_TIMER_IRQ_FLAGS      DT_IRQ(DT_INST(0, ti_am654_dmtimer), flags)

#define TI_DM_TIMER_TIDR           (TI_DM_TIMER_BASE_ADDR + 0x00)
#define TI_DM_TIMER_TIOCP_CFG		   (TI_DM_TIMER_BASE_ADDR + 0x10)
#define TI_DM_TIMER_IRQ_EOI		     (TI_DM_TIMER_BASE_ADDR + 0x20)
#define TI_DM_TIMER_IRQSTATUS_RAW  (TI_DM_TIMER_BASE_ADDR + 0x24)
#define TI_DM_TIMER_IRQSTATUS      (TI_DM_TIMER_BASE_ADDR + 0x28)
#define TI_DM_TIMER_IRQENABLE_SET  (TI_DM_TIMER_BASE_ADDR + 0x2c)
#define TI_DM_TIMER_IRQENABLE_CLR  (TI_DM_TIMER_BASE_ADDR + 0x30)
#define TI_DM_TIMER_IRQWAKEEN      (TI_DM_TIMER_BASE_ADDR + 0x34)
#define TI_DM_TIMER_TCLR           (TI_DM_TIMER_BASE_ADDR + 0x38)
#define TI_DM_TIMER_TCRR           (TI_DM_TIMER_BASE_ADDR + 0x3c)
#define TI_DM_TIMER_TLDR           (TI_DM_TIMER_BASE_ADDR + 0x40)
#define TI_DM_TIMER_TTGR           (TI_DM_TIMER_BASE_ADDR + 0x44)
#define TI_DM_TIMER_TWPS           (TI_DM_TIMER_BASE_ADDR + 0x48)
#define TI_DM_TIMER_TMAR           (TI_DM_TIMER_BASE_ADDR + 0x4c)
#define TI_DM_TIMER_TCAR1          (TI_DM_TIMER_BASE_ADDR + 0x50)
#define TI_DM_TIMER_TSICR          (TI_DM_TIMER_BASE_ADDR + 0x54)
#define TI_DM_TIMER_TCAR2          (TI_DM_TIMER_BASE_ADDR + 0x58)
#define TI_DM_TIMER_TPIR           (TI_DM_TIMER_BASE_ADDR + 0x5c)
#define TI_DM_TIMER_TNIR           (TI_DM_TIMER_BASE_ADDR + 0x60)
#define TI_DM_TIMER_TCVR           (TI_DM_TIMER_BASE_ADDR + 0x64)
#define TI_DM_TIMER_TOCR           (TI_DM_TIMER_BASE_ADDR + 0x68)
#define TI_DM_TIMER_TOWR           (TI_DM_TIMER_BASE_ADDR + 0x6c)

#define TI_DM_TIMER_IRQSTATUS_RAW_MAT_IT_FLAG_SHIFT              (0)
#define TI_DM_TIMER_IRQSTATUS_RAW_MAT_IT_FLAG_MASK               (0x00000001)

#define TI_DM_TIMER_IRQSTATUS_RAW_OVF_IT_FLAG_SHIFT              (1)
#define TI_DM_TIMER_IRQSTATUS_RAW_OVF_IT_FLAG_MASK               (0x00000002)

#define TI_DM_TIMER_IRQSTATUS_RAW_TCAR_IT_FLAG_SHIFT             (2)
#define TI_DM_TIMER_IRQSTATUS_RAW_TCAR_IT_FLAG_MASK              (0x00000004)

#define TI_DM_TIMER_IRQSTATUS_RAW_RESERVED_SHIFT                 (3)
#define TI_DM_TIMER_IRQSTATUS_RAW_RESERVED_MASK                  (0xfffffff8)

#define TI_DM_TIMER_IRQSTATUS_MAT_IT_FLAG_SHIFT                  (0)
#define TI_DM_TIMER_IRQSTATUS_MAT_IT_FLAG_MASK                   (0x00000001)

#define TI_DM_TIMER_IRQSTATUS_OVF_IT_FLAG_SHIFT                  (1)
#define TI_DM_TIMER_IRQSTATUS_OVF_IT_FLAG_MASK                   (0x00000002)

#define TI_DM_TIMER_IRQSTATUS_TCAR_IT_FLAG_SHIFT                 (2)
#define TI_DM_TIMER_IRQSTATUS_TCAR_IT_FLAG_MASK                  (0x00000004)

#define TI_DM_TIMER_IRQSTATUS_RESERVED_SHIFT                     (3)
#define TI_DM_TIMER_IRQSTATUS_RESERVED_MASK                      (0xfffffff8)

#define TI_DM_TIMER_IRQENABLE_SET_MAT_EN_FLAG_SHIFT              (0)
#define TI_DM_TIMER_IRQENABLE_SET_MAT_EN_FLAG_MASK               (0x00000001)

#define TI_DM_TIMER_IRQENABLE_SET_OVF_EN_FLAG_SHIFT              (1)
#define TI_DM_TIMER_IRQENABLE_SET_OVF_EN_FLAG_MASK               (0x00000002)

#define TI_DM_TIMER_IRQENABLE_SET_TCAR_EN_FLAG_SHIFT             (2)
#define TI_DM_TIMER_IRQENABLE_SET_TCAR_EN_FLAG_MASK              (0x00000004)

#define TI_DM_TIMER_IRQENABLE_SET_RESERVED_SHIFT                 (3)
#define TI_DM_TIMER_IRQENABLE_SET_RESERVED_MASK                  (0xfffffff8)

#define TI_DM_TIMER_IRQENABLE_CLR_MAT_EN_FLAG_SHIFT              (0)
#define TI_DM_TIMER_IRQENABLE_CLR_MAT_EN_FLAG_MASK               (0x00000001)

#define TI_DM_TIMER_IRQENABLE_CLR_OVF_EN_FLAG_SHIFT              (1)
#define TI_DM_TIMER_IRQENABLE_CLR_OVF_EN_FLAG_MASK               (0x00000002)

#define TI_DM_TIMER_IRQENABLE_CLR_TCAR_EN_FLAG_SHIFT             (2)
#define TI_DM_TIMER_IRQENABLE_CLR_TCAR_EN_FLAG_MASK              (0x00000004)

#define TI_DM_TIMER_IRQENABLE_CLR_RESERVED_SHIFT                 (3)
#define TI_DM_TIMER_IRQENABLE_CLR_RESERVED_MASK                  (0xfffffff8)

#define TI_DM_TIMER_TCLR_TCM_SHIFT                               (8)
#define TI_DM_TIMER_TCLR_TCM_MASK                                (0x00000300)
#define TI_DM_TIMER_TCLR_TCM_TCM_VALUE_0X0                       (0)
#define TI_DM_TIMER_TCLR_TCM_TCM_VALUE_0X1                       (1)
#define TI_DM_TIMER_TCLR_TCM_TCM_VALUE_0X2                       (2)
#define TI_DM_TIMER_TCLR_TCM_TCM_VALUE_0X3                       (3)

#define TI_DM_TIMER_TCLR_ST_SHIFT                                (0)
#define TI_DM_TIMER_TCLR_ST_MASK                                 (0x00000001)
#define TI_DM_TIMER_TCLR_ST_ST_VALUE_0                           (0)
#define TI_DM_TIMER_TCLR_ST_ST_VALUE_1                           (1)

#define TI_DM_TIMER_TCLR_PTV_SHIFT                               (2)
#define TI_DM_TIMER_TCLR_PTV_MASK                                (0x0000001c)

#define TI_DM_TIMER_TCLR_CE_SHIFT                                (6)
#define TI_DM_TIMER_TCLR_CE_MASK                                 (0x00000040)
#define TI_DM_TIMER_TCLR_CE_CE_VALUE_0                           (0)
#define TI_DM_TIMER_TCLR_CE_CE_VALUE_1                           (1)

#define TI_DM_TIMER_TCLR_AR_SHIFT                                (1)
#define TI_DM_TIMER_TCLR_AR_MASK                                 (0x00000002)
#define TI_DM_TIMER_TCLR_AR_AR_VALUE_0                           (0)
#define TI_DM_TIMER_TCLR_AR_AR_VALUE_1                           (1)

#define TI_DM_TIMER_TCLR_RESERVED_SHIFT                          (15)
#define TI_DM_TIMER_TCLR_RESERVED_MASK                           (0xffff8000)

#define TI_DM_TIMER_TCLR_CAPT_MODE_SHIFT                         (13)
#define TI_DM_TIMER_TCLR_CAPT_MODE_MASK                          (0x00002000)
#define TI_DM_TIMER_TCLR_CAPT_MODE_CAPT_MODE_VALUE_0             (0)
#define TI_DM_TIMER_TCLR_CAPT_MODE_CAPT_MODE_VALUE_1             (1)

#define TI_DM_TIMER_TCLR_TRG_SHIFT                               (10)
#define TI_DM_TIMER_TCLR_TRG_MASK                                (0x00000c00)
#define TI_DM_TIMER_TCLR_TRG_TRG_VALUE_0X0                       (0)
#define TI_DM_TIMER_TCLR_TRG_TRG_VALUE_0X1                       (1)
#define TI_DM_TIMER_TCLR_TRG_TRG_VALUE_0X2                       (2)
#define TI_DM_TIMER_TCLR_TRG_TRG_VALUE_0X3                       (3)

#define TI_DM_TIMER_TCLR_PT_SHIFT                                (12)
#define TI_DM_TIMER_TCLR_PT_MASK                                 (0x00001000)
#define TI_DM_TIMER_TCLR_PT_PT_VALUE_0                           (0)
#define TI_DM_TIMER_TCLR_PT_PT_VALUE_1                           (1)

#define TI_DM_TIMER_TCLR_SCPWM_SHIFT                             (7)
#define TI_DM_TIMER_TCLR_SCPWM_MASK                              (0x00000080)
#define TI_DM_TIMER_TCLR_SCPWM_SCPWM_VALUE_0                     (0)
#define TI_DM_TIMER_TCLR_SCPWM_SCPWM_VALUE_1                     (1)

#define TI_DM_TIMER_TCLR_PRE_SHIFT                               (5)
#define TI_DM_TIMER_TCLR_PRE_MASK                                (0x00000020)
#define TI_DM_TIMER_TCLR_PRE_PRE_VALUE_0                         (0)
#define TI_DM_TIMER_TCLR_PRE_PRE_VALUE_1                         (1)

#define TI_DM_TIMER_TCLR_GPO_CFG_SHIFT                           (14)
#define TI_DM_TIMER_TCLR_GPO_CFG_MASK                            (0x00004000)
#define TI_DM_TIMER_TCLR_GPO_CFG_GPO_CFG_0                       (0)
#define TI_DM_TIMER_TCLR_GPO_CFG_GPO_CFG_1                       (1UL)

#define TI_DM_TIMER_TCRR_TIMER_COUNTER_SHIFT                     (0)
#define TI_DM_TIMER_TCRR_TIMER_COUNTER_MASK                      (0xffffffff)

#define TI_DM_TIMER_TLDR_LOAD_VALUE_SHIFT                        (0)
#define TI_DM_TIMER_TLDR_LOAD_VALUE_MASK                         (0xffffffff)

#define TI_DM_TIMER_TMAR_COMPARE_VALUE_SHIFT                     (0)
#define TI_DM_TIMER_TMAR_COMPARE_VALUE_MASK                      (0xffffffff)

#endif /* ZEPHYR_DRIVERS_TIMERS_TI_DMTIMER_H_ */
