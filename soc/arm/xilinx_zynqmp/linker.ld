/*
 * Copyright (c) 2019 Lexmark International, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>

#if defined(CONFIG_SOC_XILINX_ZYNQMP_RPU)
#include <arch/arm/aarch32/cortex_r/scripts/linker.ld>
#elif defined(CONFIG_SOC_XILINX_ZYNQMP_APU)
#include <arch/arm/aarch64/scripts/linker.ld>
#endif
