/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#endif /* !_ASMLANGUAGE */

#define SOC_MCAUSE_EXP_MASK          CONFIG_RISCV_SOC_MCAUSE_EXCEPTION_MASK
#define SOC_MCAUSE_ECALL_EXP         11 /* Machine ECALL instruction */

#endif /* __SOC_H__ */
