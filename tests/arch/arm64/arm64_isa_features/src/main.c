/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <stdint.h>

ZTEST_SUITE(arm64_isa_features, NULL, NULL, NULL, NULL, NULL);

ZTEST(arm64_isa_features, test_arm64_feature_detection)
{
	uint64_t pfr0 = read_id_aa64pfr0_el1();
	uint64_t current_el = read_currentel();
	uint64_t mmfr0 = read_id_aa64mmfr0_el1();

	TC_PRINT("=== ARM64 ISA Feature Detection ===\n");
	TC_PRINT("ID_AA64PFR0_EL1:  0x%016llx\n", pfr0);
	TC_PRINT("ID_AA64MMFR0_EL1: 0x%016llx\n", mmfr0);

	/* Check for ARMv9-A specific features */
	TC_PRINT("\n=== Feature Analysis ===\n");

	/* SVE support (Scalable Vector Extension) */
	bool sve = is_sve_implemented();

	TC_PRINT("SVE support: %s\n", sve ? "YES" : "NO");

	if (sve) {
#ifdef CONFIG_ARM64_SVE
		uint32_t vl;

		__asm__("rdvl %0, #1" : "=r"(vl));
		TC_PRINT("SVE vector length: %u bytes\n", vl);

		if (vl < CONFIG_ARM64_SVE_VL_MAX) {
			TC_PRINT("Warning: CONFIG_ARM64_SVE_VL_MAX=%u while the hardware "
				 "vector length is %u.\n", CONFIG_ARM64_SVE_VL_MAX, vl);
			TC_PRINT("Warning: This will waste memory in struct k_thread.\n");
		}
#else
		TC_PRINT("Warning: CONFIG_ARM64_SVE is not set\n");
#endif
	}

	/* Current Exception Level */
	TC_PRINT("Current EL: EL%llu\n", GET_EL(current_el));

	/* Check EL support */
	TC_PRINT("EL0 AArch64: %s\n", is_el_implemented(0) ? "YES" : "NO");
	TC_PRINT("EL1 AArch64: %s\n", is_el_implemented(1) ? "YES" : "NO");
	TC_PRINT("EL2 AArch64: %s\n", is_el_implemented(2) ? "YES" : "NO");
	TC_PRINT("EL3 AArch64: %s\n", is_el_implemented(3) ? "YES" : "NO");

	/* Advanced SIMD (NEON) */
	uint64_t advsimd = (pfr0 >> ID_AA64PFR0_ADVSIMD_SHIFT) & ID_AA64PFR0_ADVSIMD_MASK;

	TC_PRINT("Advanced SIMD (NEON): %s (0x%llx)\n",
		 (advsimd == 0) ? "YES" : (advsimd == 0xf) ? "NO" : "PARTIAL", advsimd);

	/* Floating Point */
	uint64_t fp = (pfr0 >> ID_AA64PFR0_FP_SHIFT) & ID_AA64PFR0_FP_MASK;

	TC_PRINT("Floating Point: %s (0x%llx)\n",
		 (fp == 0) ? "YES" : (fp == 0xf) ? "NO" : "PARTIAL", fp);

	/* Check for additional ARMv8.5-A+ and ARMv9-A indicators */
	uint64_t pfr1 = read_id_aa64pfr1_el1();
	uint64_t isar0 = read_id_aa64isar0_el1();
	uint64_t isar1 = read_id_aa64isar1_el1();
	uint64_t isar2 = read_id_aa64isar2_el1();

	TC_PRINT("\nID_AA64PFR1_EL1:  0x%016llx\n", pfr1);
	TC_PRINT("ID_AA64ISAR0_EL1: 0x%016llx\n", isar0);
	TC_PRINT("ID_AA64ISAR1_EL1: 0x%016llx\n", isar1);
	TC_PRINT("ID_AA64ISAR2_EL1: 0x%016llx\n", isar2);

	/* Check for ARMv8.1 LSE atomics */
	uint64_t lse = (isar0 >> 20) & 0xf;

	TC_PRINT("LSE Atomics: %s (0x%llx)\n", lse ? "YES" : "NO", lse);

	/* Check for Pointer Authentication */
	uint64_t pauth_api = (isar1 >> 4) & 0xf;
	uint64_t pauth_apa = (isar1 >> 8) & 0xf;
	uint64_t pauth_gpi = (isar1 >> 28) & 0xf;
	uint64_t pauth_gpa = (isar1 >> 24) & 0xf;

	TC_PRINT("Pointer Auth (API - Address ImplDef): %s (0x%llx)\n",
		 pauth_api ? "YES" : "NO", pauth_api);
	TC_PRINT("Pointer Auth (APA - Address Arch): %s (0x%llx)\n",
		 pauth_apa ? "YES" : "NO", pauth_apa);
	TC_PRINT("Pointer Auth (GPI - Instr ImplDef): %s (0x%llx)\n",
		 pauth_gpi ? "YES" : "NO", pauth_gpi);
	TC_PRINT("Pointer Auth (GPA - Instr Arch): %s (0x%llx)\n",
		 pauth_gpa ? "YES" : "NO", pauth_gpa);

	/* Decode APA level */
	if (pauth_apa == 0x5) {
		TC_PRINT("  APA Level 5: Enhanced PAC with FPACCOMBINE\n");
	} else if (pauth_apa == 0x4) {
		TC_PRINT("  APA Level 4: Enhanced PAC with FPAC\n");
	} else if (pauth_apa == 0x3) {
		TC_PRINT("  APA Level 3: Enhanced PAC2\n");
	} else if (pauth_apa == 0x1) {
		TC_PRINT("  APA Level 1: Basic PAC\n");
	}

	/* Check for Branch Target Identification (ARMv8.5-A) */
	uint64_t bti = (pfr1 >> 0) & 0xf;

	TC_PRINT("Branch Target Identification (BTI): %s (0x%llx)\n", bti ? "YES" : "NO", bti);

	/* Check for Memory Tagging Extensions (ARMv8.5-A) */
	uint64_t mte = (pfr1 >> 8) & 0xf;

	TC_PRINT("Memory Tagging Extension (MTE): %s (0x%llx)\n", mte ? "YES" : "NO", mte);
	if (mte == 0x2) {
		TC_PRINT("  MTE Level 2: Full MTE\n");
	} else if (mte == 0x1) {
		TC_PRINT("  MTE Level 1: EL0-only\n");
	}

	/* Check for Random Number Generation (ARMv8.5-A) */
	uint64_t rndr = (pfr1 >> 16) & 0xf;

	TC_PRINT("Random Number Generation (RNDR): %s (0x%llx)\n", rndr ? "YES" : "NO", rndr);

	/* Check for Speculative Store Bypass Safe (ARMv8.5-A) */
	uint64_t ssbs = (pfr1 >> 12) & 0xf;

	TC_PRINT("Speculative Store Bypass Safe (SSBS): %s (0x%llx)\n", ssbs ? "YES" : "NO", ssbs);

	/* Check for additional ISAR2 features */
	/* WFxT - Wait For Event/Interrupt with Timeout (ARMv8.7-A) */
	uint64_t wfxt = (isar2 >> 0) & 0xf;

	TC_PRINT("WFxT (Wait with Timeout): %s (0x%llx)\n", wfxt ? "YES" : "NO", wfxt);

	/* RPRES - Reciprocal Estimate and Reciprocal Square Root Estimate */
	uint64_t rpres = (isar2 >> 4) & 0xf;

	TC_PRINT("RPRES (Reciprocal Precision): %s (0x%llx)\n", rpres ? "YES" : "NO", rpres);

	/* GPA3 - Generic Pointer Authentication using QARMA3 */
	uint64_t gpa3 = (isar2 >> 8) & 0xf;

	TC_PRINT("Pointer Auth (GPA3 - QARMA3): %s (0x%llx)\n", gpa3 ? "YES" : "NO", gpa3);

	/* APA3 - Address Pointer Authentication using QARMA3 */
	uint64_t apa3 = (isar2 >> 12) & 0xf;

	TC_PRINT("Pointer Auth (APA3 - QARMA3): %s (0x%llx)\n", apa3 ? "YES" : "NO", apa3);

	/* MOPS - Memory Copy and Memory Set instructions (ARMv8.8-A) */
	uint64_t mops = (isar2 >> 16) & 0xf;

	TC_PRINT("MOPS (Memory Copy/Set): %s (0x%llx)\n", mops ? "YES" : "NO", mops);

	/* BC - BC (Branch Consistency) model */
	uint64_t bc = (isar2 >> 20) & 0xf;

	TC_PRINT("BC (Branch Consistency): %s (0x%llx)\n", bc ? "YES" : "NO", bc);

	TC_PRINT("\n=== Architecture Assessment ===\n");
	if (sve) {
		TC_PRINT("Architecture: ARMv9-A (SVE detected)\n");
		if (bti || mte || rndr) {
			TC_PRINT("ARMv8.5-A+ features: ");
			if (bti) {
				TC_PRINT("BTI ");
			}
			if (mte) {
				TC_PRINT("MTE ");
			}
			if (rndr) {
				TC_PRINT("RNDR ");
			}
			if (ssbs) {
				TC_PRINT("SSBS ");
			}
			TC_PRINT("\n");
		}
		if (wfxt || mops || gpa3 || apa3) {
			TC_PRINT("ARMv8.7-A+ features: ");
			if (wfxt) {
				TC_PRINT("WFxT ");
			}
			if (mops) {
				TC_PRINT("MOPS ");
			}
			if (gpa3) {
				TC_PRINT("GPA3 ");
			}
			if (apa3) {
				TC_PRINT("APA3 ");
			}
			TC_PRINT("\n");
		}
	} else if (bti || mte || rndr || ssbs) {
		TC_PRINT("Architecture: ARMv8.5-A+ (BTI/MTE/RNDR detected)\n");
	} else if (lse >= 2 && (pauth_api || pauth_apa)) {
		TC_PRINT("Architecture: ARMv8.1+ with enhanced features (LSE Level 2+ and PAC)\n");
	} else if (lse || pauth_api || pauth_apa) {
		TC_PRINT("Architecture: ARMv8.1+ with ARMv9-A features (LSE/PAC)\n");
	} else {
		TC_PRINT("Architecture: ARMv8-A (no ARMv8.1+ features detected)\n");
	}

	/* Decode LSE level */
	if (lse >= 2) {
		TC_PRINT("LSE Level 2: Atomics with enhanced ordering\n");
	} else if (lse == 1) {
		TC_PRINT("LSE Level 1: Basic atomic instructions\n");
	}

	/* Basic validation that we can read system registers */
	zassert_not_equal(pfr0, 0, "ID_AA64PFR0_EL1 should not be zero");
	zassert_not_equal(current_el, 0, "CurrentEL should not be zero");

	/* We should be running in EL1 */
	zassert_equal(GET_EL(current_el), 1, "Should be running in EL1");

	/* ARMv9-A configuration validation */
	if (IS_ENABLED(CONFIG_ARMV9_A)) {
		/* ARMv9-A mandates SVE support */
		zassert_true(sve, "CONFIG_ARMV9_A enabled but no SVE detected");

		/* ARMv9-A should have enhanced security features */
		zassert_true(pauth_api || pauth_apa,
			     "CONFIG_ARMV9_A enabled but no Pointer Authentication detected");

		/* If PAC is present, validate it's enhanced (Level 3+) */
		if (pauth_apa) {
			zassert_true(pauth_apa >= 3,
				     "CONFIG_ARMV9_A enabled but PAC level too low (0x%llx) - "
				     "expected enhanced PAC (Level 3+)", pauth_apa);
		}

		/* ARMv9-A platforms should support modern atomic operations */
		zassert_true(lse >= 1,
			     "CONFIG_ARMV9_A enabled but no LSE atomics detected");
	}
}
