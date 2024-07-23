/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc_v5.h"

#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE

/* Exports */
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)

SECTION_FUNC(exception.other, __soc_save_context)

#ifdef CONFIG_SOC_ANDES_V5_PFT
	csrr t0, NDS_MXSTATUS
#endif
#ifdef CONFIG_SOC_ANDES_V5_HWDSP
	csrr t1, NDS_UCODE
#endif

#ifdef CONFIG_SOC_ANDES_V5_PFT
	sw t0, __soc_esf_t_mxstatus_OFFSET(a0)
#endif
#ifdef CONFIG_SOC_ANDES_V5_HWDSP
	sw t1, __soc_esf_t_ucode_OFFSET(a0)
#endif
	ret

SECTION_FUNC(exception.other, __soc_restore_context)

#ifdef CONFIG_SOC_ANDES_V5_PFT
	lw t0, __soc_esf_t_mxstatus_OFFSET(a0)
#endif
#ifdef CONFIG_SOC_ANDES_V5_HWDSP
	lw t1, __soc_esf_t_ucode_OFFSET(a0)
#endif

#ifdef CONFIG_SOC_ANDES_V5_PFT
	csrw NDS_MXSTATUS, t0
#endif
#ifdef CONFIG_SOC_ANDES_V5_HWDSP
	csrw NDS_UCODE, t1
#endif

#if defined(CONFIG_SOC_ANDES_V5_EXCSLVL) && defined(CONFIG_PMP_STACK_GUARD)
	/*
	 * PMP stack guard manages mstatus.MPRV and mstatus.MPP via software.
	 * Clear msavestatus.MPP1 to prevent the hardware from disabling the PMP
	 * stack guard by restoring mstatus.MPP.
	 */
	csrci NDS_MSAVESTATUS, (0x3 << 1)
#endif

	ret

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
