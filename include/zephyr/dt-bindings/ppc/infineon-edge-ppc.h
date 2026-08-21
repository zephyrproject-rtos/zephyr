/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PPC_INFINEON_EDGE_PPC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PPC_INFINEON_EDGE_PPC_H_

/*
 * PSE84 protection contexts — the fixed core / bus master each protection
 * context is bound to by the part's security policy (PSOC Edge E8x architecture
 * reference manual, "Protection context assignment" tables 98/99 for the EPC2
 * and EPC4 variants).  The bus master is the same in both variants for the
 * contexts below.  Use these as the protection-context field of an
 * "infineon,mpc" pc-configs entry, or inside INFINEON_PPC_PC() in an
 * "infineon,ppc" pc-mask.
 */
#define PSE84_PC_SE_BOOT  0 /* SE:   ROM_BOOT / RRAM_BOOT */
#define PSE84_PC_SE_RT    1 /* SE:   SE_RT_SERVICES */
#define PSE84_PC_CM33_S   2 /* CM33: extended boot / TF-M SPM / PSA RoT (secure) */
#define PSE84_PC_CM33_APP 4 /* CM33: TF-M application RoT (EPC4) / user (EPC2) */
#define PSE84_PC_CM33_NS  5 /* CM33: Non-Secure application (NSPE) */
#define PSE84_PC_CM55_NS  6 /* CM55: Non-Secure application */
#define PSE84_PC_DAP      7 /* DAP:  SYS-AP debug (MXDEBUG600) */

/*
 * Infineon PSE84 Peripheral Protection Controller (PPC) region indices, for use
 * in the "nonsecure-regions" property of "infineon,ppc" nodes.
 *
 * These mirror the cy_en_prot_region_t enum in the HAL's pse84_config.h.  PERI1
 * region IDs are offset by PROT_PERI1_START (0x10000000), which is why a raw
 * PERI1 value looks like a large number.  Only the regions currently referenced
 * from devicetree are listed; extend as needed.
 */

/* PERI0 */
#define PROT_PERI0_MAIN            0x00000000
#define PROT_PERI0_GR0_GROUP       0x00000001
#define PROT_PERI0_GR1_GROUP       0x00000002
#define PROT_PERI0_GR2_GROUP       0x00000003
#define PROT_PERI0_GR3_GROUP       0x00000004
#define PROT_PERI0_GR4_GROUP       0x00000005
#define PROT_PERI0_GR5_GROUP       0x00000006
#define PROT_PERI0_PERI_PCLK0_MAIN 0x00000010
#define PROT_PERI0_SCB0            0x0000012F
#define PROT_PERI0_SCB2            0x00000130
#define PROT_PERI0_SCB3            0x00000131
#define PROT_PERI0_SCB4            0x00000132
#define PROT_PERI0_SCB5            0x00000133
#define PROT_PERI0_SCB6            0x00000134
#define PROT_PERI0_SCB7            0x00000135
#define PROT_PERI0_SCB8            0x00000136
#define PROT_PERI0_SCB9            0x00000137
#define PROT_PERI0_SCB10           0x00000138
#define PROT_PERI0_SCB11           0x00000139
#define PROT_PERI0_SCB1            0x0000013A

/* PERI1 (indices offset by PROT_PERI1_START = 0x10000000) */
#define PROT_PERI1_MAIN 0x10000000

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PPC_INFINEON_EDGE_PPC_H_ */
