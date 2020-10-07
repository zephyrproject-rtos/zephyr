/*
 * Copyright (c) 2020 Katsuhiro Suzuki
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the QEMU virt series processor
 */

#ifndef __QEMU_RV32_VIRT_SOC_H_
#define __QEMU_RV32_VIRT_SOC_H_

#include <soc_common.h>
#include <devicetree.h>

/* Timer configuration */
#define RISCV_MSIP_BASE              0x02000000
#define RISCV_MTIMECMP_BASE          0x02004000
#define RISCV_MTIME_BASE             0x0200bff8

#endif /* __QEMU_RV32_VIRT_SOC_H_ */
