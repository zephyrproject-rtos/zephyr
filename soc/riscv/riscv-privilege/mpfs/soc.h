/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020-2021 Microchip Technology Inc
 */
#ifndef __RISCV64_MPFS_SOC_H_
#define __RISCV64_MPFS_SOC_H_

#include <soc_common.h>
#include <zephyr/devicetree.h>


/* Timer configuration */
#define RISCV_MTIMECMP_BY_HART(h)		(0x02004000ULL + (8ULL * (h)))
#define RISCV_MSIP_BASE					0x02000000

#endif /* __RISCV64_MPFS_SOC_H_ */
