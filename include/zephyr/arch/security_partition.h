/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Arch-neutral security partition API
 *
 * Abstracts the CPU-internal hardware that partitions the address space into
 * trusted and untrusted regions:
 *
 *   ARMv8-M  → SAU (Security Attribution Unit)
 *   RISC-V   → WorldGuard W-bit region table (when available)
 *   Others   → no-op stubs returning -ENOTSUP
 *
 * This API covers only the CPU-side attribution unit.  Bus-level filters
 * (ARM MPC/PPC, RISC-V IOPMP, ARM TZASC) are implementation-specific and
 * are not abstracted here.
 *
 * Only meaningful from the trusted/secure world.  Non-secure images should
 * not call these functions.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_SECURITY_PARTITION_H_
#define ZEPHYR_INCLUDE_ARCH_SECURITY_PARTITION_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Security attribution for a memory region.
 */
enum arch_security_attr {
	/**
	 * Secure: only the trusted world can access this region.
	 * On ARMv8-M this is the hardware default for addresses not covered by
	 * an enabled SAU region; listing a region as S is a no-op on that arch.
	 */
	ARCH_SECURITY_ATTR_S,

	/** Non-Secure: both the trusted and untrusted worlds can access. */
	ARCH_SECURITY_ATTR_NS,

	/**
	 * Non-Secure Callable: the untrusted world can branch into this region;
	 * the CPU switches to Secure state on the SG instruction at the entry
	 * point.  Used for the veneer region that holds __secure_call SG stubs.
	 *
	 * Architectures that do not have an NSC concept treat this as
	 * ARCH_SECURITY_ATTR_NS rather than returning an error.
	 */
	ARCH_SECURITY_ATTR_NSC,
};

/**
 * @brief Descriptor for one security-attributed address region.
 */
struct arch_security_region {
	/** Region base address.  Must satisfy arch alignment requirements
	 *  (32-byte aligned on ARMv8-M SAU).
	 */
	uintptr_t base;

	/** Region size in bytes.  Must be non-zero and arch-aligned. */
	size_t size;

	/** Security attribution to apply to this region. */
	enum arch_security_attr attr;
};

/**
 * @brief Program the security attribution hardware with a region table.
 *
 * Configures the arch security unit (SAU on ARMv8-M, WorldGuard on RISC-V,
 * …) with the supplied region descriptors.  Address ranges not covered by
 * any region default to Secure after arch_security_partition_enable() is
 * called.
 *
 * Regions are installed in the order supplied; on hardware with explicit
 * region priority the first matching region wins (same as SAU and MPU).
 *
 * @param regions  Array of region descriptors.
 * @param count    Number of entries in @p regions.
 *
 * @retval 0          Success.
 * @retval -EINVAL    A region has a misaligned base, zero size, or the
 *                    base+size would overflow.
 * @retval -ENOMEM    @p count exceeds the number of hardware regions.
 * @retval -ENOTSUP   The architecture has no security partition hardware.
 */
int arch_security_partition_configure(const struct arch_security_region *regions, uint8_t count);

/**
 * @brief Enable the security attribution hardware.
 *
 * After this call, addresses not covered by a configured NS or NSC region
 * are treated as Secure (SAU default-Secure mode).
 *
 * Call arch_security_partition_configure() before enabling.
 */
void arch_security_partition_enable(void);

/**
 * @brief Disable the security attribution hardware.
 *
 * The attribution unit is bypassed.  On ARMv8-M the IDAU alone governs
 * attribution while the SAU is disabled.  No-op on architectures without
 * security partition hardware.
 */
void arch_security_partition_disable(void);

/**
 * @brief Return the number of configurable security regions.
 *
 * @return Number of hardware regions available for configuration.
 * @return 0 if the architecture has no security partition hardware.
 */
int arch_security_partition_region_count(void);

/*
 * Entering a secure/non-secure domain (setting its vector table, granting FPU,
 * and the world-crossing hand-off) is a separate concern from address
 * attribution and lives in <zephyr/arch/secure_domain.h>
 * (arch_secure_domain_swap()).
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_SECURITY_PARTITION_H_ */
