/*
 * Copyright (c) 2024, Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>

#define __mchp_bcfg_section_altcfg Z_GENERIC_SECTION(.mchp_bcfg_altcfg)
#define __mchp_bcfg_section_altcpn Z_GENERIC_SECTION(.mchp_bcfg_altcpn)
#define __mchp_bcfg_section_mark1  Z_GENERIC_SECTION(.mchp_bcfg_mark1)
#define __mchp_bcfg_section_devcfg Z_GENERIC_SECTION(.mchp_bcfg_devcfg)
#define __mchp_bcfg_section_cpn    Z_GENERIC_SECTION(.mchp_bcfg_cpn)
#define __mchp_bcfg_section_mark2  Z_GENERIC_SECTION(.mchp_bcfg_mark2)

typedef struct {
	/* Offset: 0x08 (R/W  32) ALT USERID */
	__IO uint32_t FUSES_ALTFUSERID;
	/* Offset: 0x0C (R/W  32) ALT CFGCON4 Fuses */
	__IO uint32_t FUSES_ALTDEVCFG4;
	/* Offset: 0x10 (R/W  32) ALT CFGCON2 Fuses */
	__IO uint32_t FUSES_ALTDEVCFG2;
	/* Offset: 0x14 (R/W  32) ALT CFGCON1 Fuses */
	__IO uint32_t FUSES_ALTDEVCFG1;
	/* Offset: 0x18 (R/W  32) ALT CFGCON0 Fuses */
	__IO uint32_t FUSES_ALTDEVCFG0;
	/* Offset: 0x1C (R/W  32) ALT NVR BCFG0 */
	__IO uint32_t FUSES_ALTFBCFG0;
} fuses_resgisters_altcfg_t;

typedef struct {
	/* Offset: 0x3C (R/W  32) ALT NVR CPN Register */
	__IO uint32_t FUSES_ALTFCPN0;
} fuses_resgisters_altcpn_t;

typedef struct {
	/* Offset: 0x108 (R/W  32) USER Page USERID */
	__IO uint32_t FUSES_FUSERID;
	/* Offset: 0x10C (R/W  32) USER Page CFGCON4 Fuses */
	__IO uint32_t FUSES_DEVCFG4;
	/* Offset: 0x110 (R/W  32) USER Page CFGCON2 Fuses */
	__IO uint32_t FUSES_DEVCFG2;
	/* Offset: 0x114 (R/W  32) USER Page CFGCON1 Fuses */
	__IO uint32_t FUSES_DEVCFG1;
	/* Offset: 0x118 (R/W  32) USER Page CFGCON0 Fuses */
	__IO uint32_t FUSES_DEVCFG0;
	/* Offset: 0x11C (R/W  32) NVR BCFG User Configuration Area */
	__IO uint32_t FUSES_FBCFG0;
} fuses_resgisters_devcfg_t;

typedef struct {
	/* Offset: 0x13C (R/W  32) NVR CPN Register */
	__IO uint32_t FUSES_FCPN0;
} fuses_resgisters_cpn_t;

typedef struct {
	__IO uint32_t FUSES_MARK;
} fuses_resgisters_mark_t;

/*
 * These fields hold the full 32-bit FUSES_* register values as configured
 * via Kconfig (see Kconfig.system_config). They intentionally are not
 * decomposed bit-by-bit here; unless a design needs a non-default value,
 * the Kconfig defaults already match each register's documented reset
 * value combined with this SoC's required boot configuration.
 */
fuses_resgisters_devcfg_t __mchp_bcfg_section_devcfg __mchp_fuses_config_devcfg = {
	.FUSES_FUSERID = CONFIG_FUSES_FUSERID,
	.FUSES_DEVCFG4 = CONFIG_FUSES_DEVCFG4,
	.FUSES_DEVCFG2 = CONFIG_FUSES_DEVCFG2,
	.FUSES_DEVCFG1 = CONFIG_FUSES_DEVCFG1,
	.FUSES_DEVCFG0 = CONFIG_FUSES_DEVCFG0,
	.FUSES_FBCFG0 = CONFIG_FUSES_FBCFG0,
};

fuses_resgisters_cpn_t __mchp_bcfg_section_cpn __mchp_fuses_config_cpn = {
	.FUSES_FCPN0 = CONFIG_FUSES_FCPN0,
};

fuses_resgisters_mark_t __mchp_bcfg_section_mark1 __mchp_fuses_config_mark1 = {
	.FUSES_MARK = 0x7FFFFFFF,
};

fuses_resgisters_mark_t __mchp_bcfg_section_mark2 __mchp_fuses_config_mark2 = {
	.FUSES_MARK = 0x7FFFFFFF,
};

fuses_resgisters_altcfg_t __mchp_bcfg_section_altcfg __mchp_fuses_config_altcfg = {
	/*** ALTFUSERID ***/
	.FUSES_ALTFUSERID = FUSES_FUSERID_USER_ID(FUSES_ALTFUSERID_RESETVALUE),

	/*** ALTDEVCFG4 ***/
	.FUSES_ALTDEVCFG4 = FUSES_ALTDEVCFG4_RESETVALUE,

	/*** ALTDEVCFG2 ***/
	.FUSES_ALTDEVCFG2 = FUSES_ALTDEVCFG2_RESETVALUE,

	/*** ALTDEVCFG1 ***/
	.FUSES_ALTDEVCFG1 = FUSES_ALTDEVCFG1_RESETVALUE,

	/*** ALTDEVCFG0 ***/
	.FUSES_ALTDEVCFG0 = FUSES_ALTDEVCFG0_RESETVALUE,

	/*** ALTFBCFG0 ***/
	.FUSES_ALTFBCFG0 = FUSES_ALTFBCFG0_RESETVALUE,
};

fuses_resgisters_altcpn_t __mchp_bcfg_section_altcpn __mchp_fuses_config_altcpn = {
	/*** ALTFCPN0 ***/
	.FUSES_ALTFCPN0 = FUSES_ALTFCPN0_RESETVALUE,
};
