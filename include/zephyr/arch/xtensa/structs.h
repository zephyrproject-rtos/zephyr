/*
 * Copyright (c) Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_XTENSA_STRUCTS_H_
#define ZEPHYR_INCLUDE_XTENSA_STRUCTS_H_

/* Per CPU architecture specifics */
struct _cpu_arch {
#if defined(CONFIG_XTENSA_LAZY_HIFI_SHARING)
	atomic_ptr_val_t hifi_owner;
#elif defined(__cplusplus)
	/* Ensure this struct does not have a size of 0 which is not allowed in C++. */
	uint8_t dummy;
#endif
};

#endif /* ZEPHYR_INCLUDE_XTENSA_STRUCTS_H_ */
