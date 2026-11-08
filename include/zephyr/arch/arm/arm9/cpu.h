/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 (ARM9 / XScale) CPU definitions
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ARM9_CPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_ARM9_CPU_H_

/* AArch32 processor modes */
#define MODE_USR  0x10
#define MODE_FIQ  0x11
#define MODE_IRQ  0x12
#define MODE_SVC  0x13
#define MODE_ABT  0x17
#define MODE_UND  0x1b
#define MODE_SYS  0x1f
#define MODE_MASK 0x1f

/* CPSR bits */
#define E_BIT (1 << 9) /**< endianness (ARMv6+, RAZ here) */
#define A_BIT (1 << 8) /**< imprecise abort mask */
#define I_BIT (1 << 7) /**< IRQ mask */
#define F_BIT (1 << 6) /**< FIQ mask */
#define T_BIT (1 << 5) /**< Thumb state */

#define CPSR_M_Msk MODE_MASK
#define CPSR_M_USR MODE_USR

/*
 * System Control Register, CP15 c1 c0 0, ARMv5 layout.
 *
 * Note that ARMv5 still has the separate write buffer enable (W) and the
 * ROM/system protection bits that ARMv7 dropped.
 */
#define SCTLR_M_BIT  (1 << 0)  /**< MMU enable */
#define SCTLR_A_BIT  (1 << 1)  /**< strict alignment checking */
#define SCTLR_C_BIT  (1 << 2)  /**< data cache enable */
#define SCTLR_W_BIT  (1 << 3)  /**< write buffer enable */
#define SCTLR_B_BIT  (1 << 7)  /**< big endian */
#define SCTLR_S_BIT  (1 << 8)  /**< system protection */
#define SCTLR_R_BIT  (1 << 9)  /**< ROM protection */
#define SCTLR_Z_BIT  (1 << 11) /**< branch prediction enable */
#define SCTLR_I_BIT  (1 << 12) /**< instruction cache enable */
#define SCTLR_V_BIT  (1 << 13) /**< high exception vectors */
#define SCTLR_RR_BIT (1 << 14) /**< round robin cache replacement */

#define HIVECS SCTLR_V_BIT

/* Base address the core fetches its exception vectors from */
#define VECTOR_BASE_LOW  0x00000000U
#define VECTOR_BASE_HIGH 0xFFFF0000U

/* Size of the position independent vector block: 8 branches + 8 addresses */
#define VECTOR_TABLE_SIZE 64

/*
 * Fault Status Register (CP15 c5) encoding shared by the data and the
 * prefetch abort handlers.
 */
#define FSR_STATUS_MASK  0x0f
#define FSR_DOMAIN_SHIFT 4
#define FSR_DOMAIN_MASK  0x0f

#define FSR_ALIGNMENT                0x01
#define FSR_EXT_ABORT_TT_L1          0x0c
#define FSR_EXT_ABORT_TT_L2          0x0e
#define FSR_TRANSLATION_SECTION      0x05
#define FSR_TRANSLATION_PAGE         0x07
#define FSR_DOMAIN_SECTION           0x09
#define FSR_DOMAIN_PAGE              0x0b
#define FSR_PERMISSION_SECTION       0x0d
#define FSR_PERMISSION_PAGE          0x0f
#define FSR_EXT_ABORT_LINEFETCH_S    0x04
#define FSR_EXT_ABORT_LINEFETCH_P    0x06
#define FSR_EXT_ABORT_NONLINEFETCH_S 0x08
#define FSR_EXT_ABORT_NONLINEFETCH_P 0x0a

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ARM9_CPU_H_ */
