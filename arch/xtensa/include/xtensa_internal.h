/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_INTERNAL_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_INTERNAL_H_

void z_xtensa_dump_stack(const z_arch_esf_t *stack);
char *z_xtensa_exccause(unsigned int cause_code);

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_INTERNAL_H_ */
