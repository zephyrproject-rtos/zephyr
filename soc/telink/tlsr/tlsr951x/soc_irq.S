/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>

#include <soc.h>

#define NDS_MXSTATUS                 0x7C4

/* Exports */
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

SECTION_FUNC(exception.other, __soc_save_context)

#ifdef CONFIG_TELINK_B91_PFT_ARCH
	csrr t0, NDS_MXSTATUS
#endif
#ifdef __riscv_dsp
	csrr t1, ucode
#endif

#ifdef CONFIG_TELINK_B91_PFT_ARCH
	sw t0, __soc_esf_t_mxstatus_OFFSET(a0)
#endif
#ifdef __riscv_dsp
	sw t1, __soc_esf_t_ucode_OFFSET(a0)
#endif
	ret

SECTION_FUNC(exception.other, __soc_restore_context)

#ifdef CONFIG_TELINK_B91_PFT_ARCH
	lw t0, __soc_esf_t_mxstatus_OFFSET(a0)
#endif
#ifdef __riscv_dsp
	lw t1, __soc_esf_t_ucode_OFFSET(a0)
#endif

#ifdef CONFIG_TELINK_B91_PFT_ARCH
	csrw NDS_MXSTATUS, t0
#endif
#ifdef __riscv_dsp
	csrw ucode, t1
#endif
	ret

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
