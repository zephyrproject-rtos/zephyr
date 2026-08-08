/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A/R outer (L2) cache maintenance hooks (arch-internal).
 *
 * These declarations are deliberately NOT part of the public cache API. The
 * arch d-cache range operations in cortex_a_r/cache.c pair every L1 maintenance
 * op with the matching outer-cache op through the z_arm_outer_cache_* hooks.
 * They default to __weak no-ops and are overridden at link time by an
 * outer-cache driver (e.g. cortex_a_r/pl310.c) when CONFIG_OUTER_CACHE is set.
 *
 * The only entry point a SoC has to invoke explicitly is z_pl310_early_enable(),
 * because the controller must be brought up from the SoC reset hook while the
 * MMU and L1 cache are still off - too early for the normal device init path.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_OUTERCACHE_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_OUTERCACHE_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_OUTER_CACHE

/**
 * @brief Flush an address range in the outer cache.
 *
 * Paired with the L1 flush by the arch d-cache range operations. Provided by
 * the outer-cache driver; defaults to a no-op when no controller is present.
 */
void z_arm_outer_cache_flush_range(void *addr, size_t size);

/**
 * @brief Invalidate an address range in the outer cache.
 *
 * Paired with the L1 invalidate by the arch d-cache range operations. Provided
 * by the outer-cache driver; defaults to a no-op when no controller is present.
 */
void z_arm_outer_cache_invd_range(void *addr, size_t size);

/**
 * @brief Flush and invalidate an address range in the outer cache.
 *
 * Paired with the L1 flush-and-invalidate by the arch d-cache range operations.
 * Provided by the outer-cache driver; defaults to a no-op when no controller is
 * present.
 */
void z_arm_outer_cache_flush_and_invd_range(void *addr, size_t size);

#endif /* CONFIG_OUTER_CACHE */

#ifdef CONFIG_ARM_PL310

/**
 * @brief Bring up the PL310 outer cache with the MMU and L1 still off.
 *
 * Reads base address and platform straps from the enabled arm,pl310-cache
 * devicetree node. Intended to be called from a SoC reset hook before the MMU
 * is enabled.
 */
void z_pl310_early_enable(void);

#endif /* CONFIG_ARM_PL310 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_CORTEX_A_R_OUTERCACHE_H_ */
