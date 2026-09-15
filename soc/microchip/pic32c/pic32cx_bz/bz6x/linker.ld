/*
 * Copyright (c) 2024, Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

GROUP_START(FLASH_BCFG)

SECTION_PROLOGUE(.mchp_bcfg_altcfg,0x810e88,)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bcfg_altcfg))
}
SECTION_PROLOGUE(.mchp_bcfg_altcpn,0x810ebc,)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bcfg_altcpn))
}
SECTION_PROLOGUE(.mchp_bcfg_mark1,0x810edc,)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bcfg_mark1))
}
SECTION_PROLOGUE(.mchp_bcfg_devcfg,0x810f88,)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bcfg_devcfg))
}
SECTION_PROLOGUE(.mchp_bcfg_cpn,0x810fbc,)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bcfg_cpn))
}

GROUP_END(FLASH_BCFG)


#ifdef CONFIG_BT
SECTION_PROLOGUE(.mchp_bt,0x20018C00(NOLOAD),)
{
    . = ALIGN(4);
    KEEP(*(.mchp_bt))
}
#endif

SECTION_PROLOGUE (.pbss,0x44014000, )
{
    KEEP(*(SORT_BY_NAME(".pbss*")))
}
