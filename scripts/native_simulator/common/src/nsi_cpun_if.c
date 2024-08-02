/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nsi_cpu_if.h"

/*
 * These trampolines forward a call from the runner into the corresponding embedded CPU hook
 * for ex., nsif_cpun_boot(4) -> nsif_cpu4_boot()
 */

TRAMPOLINES(nsif_cpu, _pre_cmdline_hooks)
TRAMPOLINES(nsif_cpu, _pre_hw_init_hooks)
TRAMPOLINES(nsif_cpu, _boot)
TRAMPOLINES_i_(nsif_cpu, _cleanup)
TRAMPOLINES(nsif_cpu, _irq_raised)
TRAMPOLINES(nsif_cpu, _irq_raised_from_sw)
TRAMPOLINES_i_vp(nsif_cpu, _test_hook)
