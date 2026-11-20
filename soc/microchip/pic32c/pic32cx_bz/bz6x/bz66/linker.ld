/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BCFG_BASE_ADDR        DT_REG_ADDR(DT_NODELABEL(bcfg))
#define PBSS_BASE_ADDR        DT_REG_ADDR(DT_NODELABEL(retention))

#define BCFG_ALTCFG_OFFSET    0x08
#define BCFG_ALTCPN_OFFSET    0x3C
#define BCFG_MARK1_OFFSET     0x5C
#define BCFG_DEVCFG_OFFSET    0x108
#define BCFG_CPN_OFFSET       0x13C

#define BCFG_ALTCFG   (BCFG_BASE_ADDR + BCFG_ALTCFG_OFFSET)
#define BCFG_ALTCPN   (BCFG_BASE_ADDR + BCFG_ALTCPN_OFFSET)
#define BCFG_MARK1    (BCFG_BASE_ADDR + BCFG_MARK1_OFFSET)
#define BCFG_DEVCFG   (BCFG_BASE_ADDR + BCFG_DEVCFG_OFFSET)
#define BCFG_CPN      (BCFG_BASE_ADDR + BCFG_CPN_OFFSET)

GROUP_START(FLASH_BCFG)

SECTION_PROLOGUE(.mchp_bcfg_altcfg,BCFG_ALTCFG,)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bcfg_altcfg))
}

SECTION_PROLOGUE(.mchp_bcfg_altcpn,BCFG_ALTCPN,)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bcfg_altcpn))
}

SECTION_PROLOGUE(.mchp_bcfg_mark1,BCFG_MARK1,)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bcfg_mark1))
}

SECTION_PROLOGUE(.mchp_bcfg_devcfg,BCFG_DEVCFG,)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bcfg_devcfg))
}

SECTION_PROLOGUE(.mchp_bcfg_cpn,BCFG_CPN,)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bcfg_cpn))
}

GROUP_END(FLASH_BCFG)

SECTION_PROLOGUE (.pbss,PBSS_BASE_ADDR, )
{
    KEEP(*(SORT_BY_NAME(".pbss*")))
}
