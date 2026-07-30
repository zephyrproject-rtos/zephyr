/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PPC_INFINEON_PSC3_PPC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PPC_INFINEON_PSC3_PPC_H_

/*
 * PSC3 protection contexts — assigned per firmware/software layer by the part's
 * security policy (PSOC Control C3 architecture reference manual, "Protection
 * context assignment to firmware layers", table 96).  PSC3 is a single-CM33
 * part, so the contexts are layers rather than distinct cores.  Use these as
 * the protection-context field of an "infineon,mpc" pc-configs entry, or inside
 * INFINEON_PPC_PC() in an "infineon,ppc" pc-mask.
 */
#define PSC3_PC_BOOT    0 /* ROM_BOOT / FLASH_BOOT */
#define PSC3_PC_PROT_FW 1 /* Protected firmware */
#define PSC3_PC_TFM     2 /* OEM bootloader / TF-M (secure) */
#define PSC3_PC_NS_APP  6 /* OEM Non-Secure application (PC3..PC6) */
#define PSC3_PC_DAP     7 /* SYS-AP debug (MXDEBUG600) */

/*
 * PSC3 PPC peripheral-protection region indices (cy_en_prot_region_t, mirroring
 * the HAL psc3_config.h enum).  Used in the "infineon,ppc" node's
 * nonsecure-regions property to open PERI0 peripherals for Non-Secure access.
 */

/* PERI0 */
#define PROT_PERI0_MAIN            0
#define PROT_PERI0_GR1_GROUP       2
#define PROT_PERI0_GR2_GROUP       3
#define PROT_PERI0_GR3_GROUP       4
#define PROT_PERI0_GR4_GROUP       5
#define PROT_PERI0_GR5_GROUP       6
#define PROT_PERI0_PERI_PCLK0_MAIN 16
#define PROT_SCB0                  191
#define PROT_SCB1                  192
#define PROT_SCB2                  193
#define PROT_SCB3                  194
#define PROT_SCB4                  195
#define PROT_SCB5                  196

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PPC_INFINEON_PSC3_PPC_H_ */
