/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Arch-neutral secure-domain entry API
 *
 * A secure domain is a hardware-isolated execution context that the current
 * (trusted) code can hand control to — for example the Non-Secure world on an
 * ARMv8-M TrustZone core, a World on RISC-V WorldGuard, or a Trust Domain on
 * platforms that use that term.  arch_secure_domain_swap() applies the domain's
 * entry setup and transfers control into it.
 *
 * This is distinct from the address-attribution API in
 * <zephyr/arch/security_partition.h> (SAU/PMP region tables): that decides
 * which addresses belong to which domain; this enters a domain.  Bus-level
 * filters (MPC/PPC/TZASC/WorldGuard checkers) are separate driver classes.
 *
 * The ARMv8-M backend implements a one-way hand-off: arch_secure_domain_swap()
 * does not return.  Only meaningful from the trusted world.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_SECURE_DOMAIN_H_
#define ZEPHYR_INCLUDE_ARCH_SECURE_DOMAIN_H_

#include <stdint.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief struct arch_secure_domain flag: grant the domain access to the FPU. */
#define ARCH_SECURE_DOMAIN_ENABLE_FPU (1U << 0)

/**
 * @brief Description of a secure domain to enter.
 */
struct arch_secure_domain {
	/** Entry address — where the domain begins executing. */
	uintptr_t entry;
	/** Initial stack pointer for the domain. */
	uintptr_t stack;
	/**
	 * Vector/interrupt table base to install for the domain, or 0 to leave
	 * the domain's current vector table unchanged.
	 */
	uintptr_t vector_table;
	/** Bitwise OR of ARCH_SECURE_DOMAIN_* flags. */
	uint32_t flags;
};

/**
 * @brief Hand control to a secure domain (does not return).
 *
 * Applies @p domain's entry setup (vector table, FPU access, stack pointer) and
 * transfers control to @p domain->entry.  On ARMv8-M this is a one-way hand-off
 * to the Non-Secure world.
 *
 * All isolation setup (address attribution, peripheral hand-off) must be
 * complete before calling; on a non-returning hand-off the caller does not run
 * again.  Only meaningful from the trusted world.
 *
 * @param domain  Secure domain to enter.
 */
FUNC_NORETURN void arch_secure_domain_swap(const struct arch_secure_domain *domain);

/*
 * Backend hooks used by arch_secure_domain_swap().  These are weak so a SoC can
 * override them for silicon that deviates from the architectural sequence
 * (e.g. PSE84, whose BXNS raises INVTRAN).  Application code should call
 * arch_secure_domain_swap(), not these directly.
 */

/** @brief Install @p base as the entered domain's vector table base. */
void z_arch_secure_domain_set_vector_table(uintptr_t base);

/** @brief Transfer control to @p entry using @p stack; does not return. */
FUNC_NORETURN void z_arch_secure_domain_enter(uintptr_t entry, uintptr_t stack);

/**
 * @brief Abort the current secure call (does not return).
 *
 * Called by the Z_SECURE_VERIFY() validation macros (see
 * <zephyr/internal/secure_call_handler.h>) when a Non-Secure caller passes an
 * argument that fails a bounds/attribution check — a confused-deputy attempt.
 * The __weak default halts; a SoC may override it with a custom oops/reset
 * handler.
 */
FUNC_NORETURN void arch_secure_call_oops(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_SECURE_DOMAIN_H_ */
