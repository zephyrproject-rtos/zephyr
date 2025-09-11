/*
 * Copyright (c) 2021 Andes Technology Corporation
 * Copyright (c) 2025 Egis Technology Inc.
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

#ifdef CONFIG_SOC_EGIS_ET171_PFT
	csrr t0, NDS_MXSTATUS
#endif
#ifdef CONFIG_SOC_EGIS_ET171_HWDSP
	csrr t1, NDS_UCODE
#endif

#ifdef CONFIG_SOC_EGIS_ET171_PFT
	sw t0, __soc_esf_t_mxstatus_OFFSET(a0)
#endif
#ifdef CONFIG_SOC_EGIS_ET171_HWDSP
	sw t1, __soc_esf_t_ucode_OFFSET(a0)
#endif
	ret

SECTION_FUNC(exception.other, __soc_restore_context)

#ifdef CONFIG_SOC_EGIS_ET171_PFT
	lw t0, __soc_esf_t_mxstatus_OFFSET(a0)
#endif
#ifdef CONFIG_SOC_EGIS_ET171_HWDSP
	lw t1, __soc_esf_t_ucode_OFFSET(a0)
#endif

#ifdef CONFIG_SOC_EGIS_ET171_PFT
	csrw NDS_MXSTATUS, t0
#endif
#ifdef CONFIG_SOC_EGIS_ET171_HWDSP
	csrw NDS_UCODE, t1
#endif
	ret

#endif /* CONFIG_RISCV_SOC_CONTEXT_SAVE */
