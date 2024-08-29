/*
 * Copyright(c) 2024, CISPA Helmholtz Center for Information Security
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * OpenHwGroup CVA6 declarations
 */

#ifndef ZEPHYR_SOC_RISCV_OPENHWGROUP_CVA6_H
#define ZEPHYR_SOC_RISCV_OPENHWGROUP_CVA6_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CVA6 provides two custom CSRs for cache management:
 * CSR 7C1 controls the data cache, CSR 7C0 controls the instruction cache.
 * The least significant bit of the CSRs can be written to enable or disable the cache.
 * Writing a value of 1 means enabling the cache, writing 0 disables it.
 * After reset, both caches are enabled by default.
 *
 */

#define SOC_CVA6_CUSTOM_CSR_DCACHE 0x7C1
#define SOC_CVA6_CUSTOM_CSR_ICACHE 0x7C0

#define SOC_CVA6_CUSTOM_CSR_DCACHE_ENABLE  0x1
#define SOC_CVA6_CUSTOM_CSR_DCACHE_DISABLE 0x0

#define SOC_CVA6_CUSTOM_CSR_ICACHE_ENABLE  0x1
#define SOC_CVA6_CUSTOM_CSR_ICACHE_DISABLE 0x0

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_RISCV_OPENHWGROUP_CVA6_H */
