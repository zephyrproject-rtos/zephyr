/*
 * Copyright (c) BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RISCV_STRUCTS_H_
#define ZEPHYR_INCLUDE_RISCV_STRUCTS_H_

/* Per CPU architecture specifics */
struct _cpu_arch {
#ifdef CONFIG_USERSPACE
	unsigned long user_exc_sp;
	unsigned long user_exc_tmp0;
	unsigned long user_exc_tmp1;
#endif
};

#endif /* ZEPHYR_INCLUDE_RISCV_STRUCTS_H_ */
