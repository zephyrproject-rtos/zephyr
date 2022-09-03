/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Define WCH QingKe core specific CSR and related definitions.
 *
 * This header contains WCH QingKe core specific definitions.
 * Use arch/riscv/csr.h for RISC-V standard CSR and definitions.
 */

#ifndef ZEPHYR_SOC_RISCV_RISCV_PRIVILEGE_COMMON_WCH_QINGKE_CSR_H_
#define ZEPHYR_SOC_RISCV_RISCV_PRIVILEGE_COMMON_WCH_QINGKE_CSR_H_

#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CSR_GINTENR  0x800U
#define CSR_INTSYSCR 0x804U
#define CSR_CORECFGR 0xBC0U

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_RISCV_RISCV_PRIVILEGE_COMMON_WCH_QINGKE_CSR_H_ */
