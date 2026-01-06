/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets.h>
#include <zephyr/toolchain.h>

#include <soc.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* Exports */
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)

#if defined(__riscv_vector)
SECTION_FUNC(exception.other, __soc_save_context)
	csrr t0, vl
	sd t0, __soc_esf_t_vl_OFFSET(a0)
	csrr t0, vtype
	sd t0, __soc_esf_t_vtype_OFFSET(a0)
	csrr t0, vstart
	sd t0, __soc_esf_t_vstart_OFFSET(a0)
	csrr t0, vxsat
	sd t0, __soc_esf_t_vxsat_OFFSET(a0)
	csrr t0, vxrm
	sd t0, __soc_esf_t_vxrm_OFFSET(a0)

	addi     t1, a0, __soc_esf_t_v_OFFSET
	csrr     t0, vlenb
	slli     t0, t0, 3
	vsetvli  zero, zero, e8, m8
#if (__riscv_v == 7000)
	vsb.v    v0, (t1)
	add      t1, t1, t0
	vsb.v    v8, (t1)
	add      t1, t1, t0
	vsb.v    v16, (t1)
	add      t1, t1, t0
	vsb.v    v24, (t1)
#elif (__riscv_v == 1000000)
	vs8r.v   v0, (t1)
	add      t1, t1, t0
	vs8r.v   v8, (t1)
	add      t1, t1, t0
	vs8r.v   v16, (t1)
	add      t1, t1, t0
	vs8r.v   v24, (t1)
#endif
	ret

SECTION_FUNC(exception.other, __soc_restore_context)
	addi     t3, a0, __soc_esf_t_v_OFFSET
	csrr     t0, vlenb
	slli     t0, t0, 3
	vsetvli  zero, zero, e8, m8
#if (__riscv_v == 7000)
	vlb.v    v0, (t3)
	add      t3, t3, t0
	vlb.v    v8, (t3)
	add      t3, t3, t0
	vlb.v    v16, (t3)
	add      t3, t3, t0
	vlb.v    v24, (t3)
	add      t3, t3, t0
#elif (__riscv_v == 1000000)
	vl8r.v   v0, (t3)
	add      t3, t3, t0
	vl8r.v   v8, (t3)
	add      t3, t3, t0
	vl8r.v   v16, (t3)
	add      t3, t3, t0
	vl8r.v   v24, (t3)
	add      t3, t3, t0
#endif
	ld       t0, __soc_esf_t_vl_OFFSET(a0)
	ld       t1, __soc_esf_t_vtype_OFFSET(a0)
	vsetvl   zero, t0, t1
	ld       t0, __soc_esf_t_vstart_OFFSET(a0)
	csrw     vstart, t0
	ld       t0, __soc_esf_t_vxsat_OFFSET(a0)
	csrw     vxsat, t0
	ld       t0, __soc_esf_t_vxrm_OFFSET(a0)
	csrw     vxrm, t0
	ret
#endif /* __riscv_vector */

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
