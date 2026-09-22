/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_TRICORE_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_TRICORE_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <zephyr/offsets.h>

#define _thread_offset_to_saved_pcxi                                                               \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_pcxi_OFFSET)

#define _cpu_offset_to_to_reclaim (___cpu_t_arch_OFFSET + ___cpu_arch_t_to_reclaim_OFFSET)

#endif
