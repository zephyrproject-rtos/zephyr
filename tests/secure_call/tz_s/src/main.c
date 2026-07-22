/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Secure-image entry point for the __secure_call TrustZone integration test.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/security_partition.h>
#include <zephyr/arch/secure_domain.h>
#if defined(CONFIG_MPC)
#include <zephyr/drivers/mpc/mpc.h>
#endif
#if defined(CONFIG_PPC)
#include <zephyr/drivers/ppc/ppc.h>
#endif
#include <cmsis_core.h>

/*
 * Non-Secure image vector table location (board-specific, from Kconfig):
 *   NS_IMAGE_BASE     — Non-Secure alias; the Secure image points the NS VTOR
 *                       here and reads the NS initial SP/PC from it.
 *   NS_IMAGE_BASE_SA  — Secure alias of the same location; used to read the NS
 *                       vector table before the MPC restricts Secure access.
 */
#define NS_IMAGE_BASE    CONFIG_SECURE_CALL_NS_IMAGE_BASE
#define NS_IMAGE_BASE_SA CONFIG_SECURE_CALL_NS_IMAGE_BASE_SECURE_ALIAS

/*
 * Build the SAU region table from the board's "zephyr,security-partition" node.
 * The zephyr,attr enum is ordered to match enum arch_security_attr, so its
 * DT_ENUM_IDX maps straight onto the .attr field (asserted below).
 */
#define SAU_REGION_ENTRY(node)                                                                     \
	{                                                                                          \
		.base = DT_REG_ADDR(node),                                                         \
		.size = DT_REG_SIZE(node),                                                         \
		.attr = (enum arch_security_attr)DT_ENUM_IDX(node, zephyr_attr),                   \
	},

static void sau_setup(void)
{
	static const struct arch_security_region regions[] = {
		DT_FOREACH_CHILD(DT_NODELABEL(security_partition), SAU_REGION_ENTRY)};

	BUILD_ASSERT((int)ARCH_SECURITY_ATTR_S == 0);
	BUILD_ASSERT((int)ARCH_SECURITY_ATTR_NS == 1);
	BUILD_ASSERT((int)ARCH_SECURITY_ATTR_NSC == 2);

	arch_security_partition_configure(regions, ARRAY_SIZE(regions));
	arch_security_partition_enable();
	__DSB();
	__ISB();
}

int main(void)
{
	/* Read NS vector table via S alias BEFORE MPC blocks Secure access to those blocks. */
	volatile uint32_t *ns_vt = (volatile uint32_t *)(uintptr_t)NS_IMAGE_BASE_SA;
	uint32_t ns_sp = ns_vt[0];
	uint32_t ns_pc = ns_vt[1];

	printk("S: NS image pc=0x%08x sp=0x%08x\n", ns_pc, ns_sp);

#if defined(CONFIG_MPC)
	printk("S: configuring MPC\n");
	mpc_configure_all();
	__DSB();
	__ISB();
#endif

	printk("S: configuring SAU\n");
	sau_setup();

	/*
	 * The Non-Secure image, as a secure domain to hand control to: entry/stack
	 * come from its vector table, its vector-table base is installed as the NS
	 * VTOR, and it is granted FPU access (the NS image is built hard-float).
	 */
	const struct arch_secure_domain ns_domain = {
		.entry = ns_pc,
		.stack = ns_sp,
		.vector_table = NS_IMAGE_BASE,
		.flags = ARCH_SECURE_DOMAIN_ENABLE_FPU,
	};

#if defined(CONFIG_PPC)
	/*
	 * Grant the NS protection context access to the peripherals it needs (the
	 * shared console among them) via the PPC driver (DT-driven; on PSE84 the
	 * curated "M33_M55" region list, on the SSE-200 the APBNSPPCEXP1 mask).
	 *
	 * The console is shared with the Secure image: once it becomes non-secure,
	 * any Secure access to its secure alias bus-faults.  This MUST be the last
	 * Secure printk — arch_secure_domain_swap() below touches only CPU/system
	 * registers (and the MXCM33 VTOR shadow on PSE84), never the shared console.
	 */
	printk("S: configuring PPC then entering NS (no more S output)\n");
	ppc_configure_ns_all();
	__DSB();
#endif

	/*
	 * Hand control to the Non-Secure domain (does not return).  The entry
	 * mechanism — BXNS, or the SVC exception-return flow for silicon with the
	 * INVTRAN deviation (CONFIG_ARM_SECURE_DOMAIN_ENTRY_SVC) — is chosen by
	 * the arch backend of arch_secure_domain_swap().
	 */
	arch_secure_domain_swap(&ns_domain);
	__builtin_unreachable();
}
