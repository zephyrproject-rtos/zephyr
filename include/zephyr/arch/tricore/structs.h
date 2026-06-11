/*
 * Copyright (c) 2026 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TriCore per-CPU arch structure
 */

#ifndef ZEPHYR_INCLUDE_TRICORE_STRUCTS_H_
#define ZEPHYR_INCLUDE_TRICORE_STRUCTS_H_

/** @cond INTERNAL_HIDDEN */

/* Per CPU architecture specifics */
struct _cpu_arch {
	/** Thread to reclaim CSAs from during next context switch */
	struct k_thread *to_reclaim;
};

/** @endcond */

#endif /* ZEPHYR_INCLUDE_TRICORE_STRUCTS_H_ */
