/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RH850_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_RH850_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <zephyr/offsets.h>

#define _cpu_offset_to_esf (___cpu_t_arch_OFFSET + ___cpu_arch_t_esf_OFFSET)

#define _esf_r1    __struct_arch_esf_r1_OFFSET
#define _esf_r2    __struct_arch_esf_r2_OFFSET
#define _esf_r4    __struct_arch_esf_r4_OFFSET
#define _esf_eipc  __struct_arch_esf_eipc_OFFSET
#define _esf_eipsw __struct_arch_esf_eipsw_OFFSET
#define _esf_fpsr  __struct_arch_esf_fpsr_OFFSET
#define _esf_fpepc __struct_arch_esf_fpepc_OFFSET
#define _esf_ctpc  __struct_arch_esf_ctpc_OFFSET
#define _esf_ctpsw __struct_arch_esf_ctpsw_OFFSET

#endif /* ZEPHYR_ARCH_RH850_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
