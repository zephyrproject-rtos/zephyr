/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_RISCV_CUSTOM_CSR_CONTEXT_H
#define ZEPHYR_RISCV_CUSTOM_CSR_CONTEXT_H

#include <zephyr/autoconf.h>

#if defined(__riscv_vector)

/* clang-format off */

/* there have 32 numbers vector registers. attention: don't contain csr of vector */
#define CTX_VECTOR_REG_NR  (32 * (CONFIG_XUANTIE_RISCV_VLENB_LEN / 8) / sizeof(unsigned long))

#define SOC_ESF_MEMBERS     \
	unsigned long vl;       \
	unsigned long vtype;	\
	unsigned long vstart;	\
	unsigned long vxsat;	\
	unsigned long vxrm;		\
	unsigned long v[CTX_VECTOR_REG_NR]

#define SOC_ESF_INIT	\
	0,					\
	0,					\
	0,					\
	0,					\
	0,					\
	{[0 ... (CTX_VECTOR_REG_NR - 1)] = 0x12345678}

/* clang-format on */

#endif /* __riscv_vector */

#endif /* ZEPHYR_RISCV_CUSTOM_CSR_CONTEXT_H */
