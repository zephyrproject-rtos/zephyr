/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM L2C-310 (PL310) outer-cache driver public entry point.
 *
 * The register maintenance ops are exposed through the arch outer-cache hooks
 * (z_arm_outer_cache_*, see <zephyr/arch/cache.h>) and need no direct caller.
 * The only thing a SoC has to invoke explicitly is the early bring-up, because
 * the controller must be enabled from the SoC reset hook while the MMU and L1
 * cache are still off - too early for the normal device init path.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CACHE_PL310_H_
#define ZEPHYR_INCLUDE_DRIVERS_CACHE_PL310_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bring up the PL310 outer cache with the MMU and L1 still off.
 *
 * Reads base address and platform straps from the enabled arm,pl310-cache
 * devicetree node. Intended to be called from a SoC reset hook before the MMU
 * is enabled. No-op if no PL310 node is enabled.
 */
void z_pl310_early_enable(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CACHE_PL310_H_ */
