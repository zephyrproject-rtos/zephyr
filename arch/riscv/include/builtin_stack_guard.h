/*
 * Copyright (c) 2026 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BUILTIN_STACK_GUARD_H_
#define BUILTIN_STACK_GUARD_H_

#include <zephyr/arch/exception.h>

void z_riscv_builtin_stack_guard_init(void);
void z_riscv_builtin_stack_guard_enable(void);
void z_riscv_builtin_stack_guard_disable(void);
void z_riscv_builtin_stack_guard_config(unsigned long start, unsigned long size);
bool z_riscv_builtin_stack_guard_is_fault(struct arch_esf *esf);

#endif /* BUILTIN_STACK_GUARD_H_ */
