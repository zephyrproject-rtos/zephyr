/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Extra definitions required for CONFIG_RISCV_SOC_OFFSETS.
 */

#ifndef SOC_XUANTIE_RISCV_SOC_OFFSETS_H_
#define SOC_XUANTIE_RISCV_SOC_OFFSETS_H_

#ifdef CONFIG_RISCV_SOC_OFFSETS

/* clang-format off */

#if defined(__riscv_vector)
#define GEN_SOC_OFFSET_SYMS()				\
	GEN_OFFSET_SYM(soc_esf_t, vl);			\
	GEN_OFFSET_SYM(soc_esf_t, vtype);		\
	GEN_OFFSET_SYM(soc_esf_t, vstart);		\
	GEN_OFFSET_SYM(soc_esf_t, vxsat);		\
	GEN_OFFSET_SYM(soc_esf_t, vxrm);		\
	GEN_OFFSET_SYM(soc_esf_t, v)
#endif /* __riscv_vector */

/* clang-format on */

#endif /* CONFIG_RISCV_SOC_OFFSETS */

#endif /* SOC_XUANTIE_RISCV_SOC_OFFSETS_H_*/
