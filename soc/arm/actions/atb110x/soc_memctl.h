/*
 * Copyright (c) 2019 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Memory controller configuration macros for ATB110X
 */

#ifndef _ACTIONS_SOC_MEM_CTL_H_
#define _ACTIONS_SOC_MEM_CTL_H_

#define MEM_CTL      (MEMCTL_REG_BASE + 0x00)
#define VECTOR_BASE  (MEMCTL_REG_BASE + 0x04)

#define MEM_CTL_VECTOR_TABLE_SEL  BIT(0)

#endif /* _ACTIONS_SOC_MEM_CTL_H_ */
