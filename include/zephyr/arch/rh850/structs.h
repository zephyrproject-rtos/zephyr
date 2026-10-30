/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RH850 per-CPU arch structure
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_STRUCTS_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_STRUCTS_H_

/** @cond INTERNAL_HIDDEN */

struct arch_esf;

struct _cpu_arch {
	struct arch_esf *esf;
};

/** @endcond */

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_STRUCTS_H_ */
