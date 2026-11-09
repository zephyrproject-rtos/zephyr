/*
 * Copyright (c) 2025 HPMicro
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HPM_ZEPHYR_SOC_HPM5300_SOC_H_
#define HPM_ZEPHYR_SOC_HPM5300_SOC_H_

#include <zephyr/devicetree.h>

#define RISCV_MTIME_BASE    DT_REG_ADDR_BY_IDX(DT_NODELABEL(mtimer), 0)
#define RISCV_MTIMECMP_BASE DT_REG_ADDR_BY_IDX(DT_NODELABEL(mtimer), 1)

#define SOC_MCAUSE_ECALL_EXP      11
#define SOC_MCAUSE_USER_ECALL_EXP 8

#endif /* HPM_ZEPHYR_SOC_HPM5300_SOC_H_ */
