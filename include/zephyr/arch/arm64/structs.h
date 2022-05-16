/*
 * Copyright (c) BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARM64_STRUCTS_H_
#define ZEPHYR_INCLUDE_ARM64_STRUCTS_H_

/* Per CPU architecture specifics */
struct _cpu_arch {
#ifdef CONFIG_FPU_SHARING
	struct k_thread *fpu_owner;
#endif
};

#endif /* ZEPHYR_INCLUDE_ARM64_STRUCTS_H_ */
