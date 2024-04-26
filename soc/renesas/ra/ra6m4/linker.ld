/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/devicetree_regions.h>

#if DT_NODE_EXISTS(DT_NODELABEL(ofs_setting))
#define OPTION_SETTING_START 	DT_REG_ADDR(DT_NODELABEL(ofs_setting))
#define OPTION_SETTING_LENGTH 	DT_REG_SIZE(DT_NODELABEL(ofs_setting))
#define OPTION_SETTING_S_START 	DT_REG_ADDR(DT_NODELABEL(ofs_setting_s))
#define OPTION_SETTING_S_LENGTH DT_REG_SIZE(DT_NODELABEL(ofs_setting_s))
#else
#define OPTION_SETTING_START 	0x00000000
#define OPTION_SETTING_LENGTH 	0x0
#define OPTION_SETTING_S_START 	0x80000000
#define OPTION_SETTING_S_LENGTH 0x0
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(id_code))
#define ID_CODE_START			DT_REG_ADDR(DT_NODELABEL(id_code))
#define ID_CODE_LENGTH			DT_REG_SIZE(DT_NODELABEL(id_code))
#else
#define ID_CODE_START 			0x00000000
#define ID_CODE_LENGTH 			0x0
#endif

NS_OFFSET_START = DEFINED(NS_OFFSET_START) ? NS_OFFSET_START : 0;
NS_IMAGE_OFFSET = DEFINED(PROJECT_NONSECURE) ? NS_OFFSET_START : 0;
OPTION_SETTING_START_NS = DEFINED(PROJECT_NONSECURE) ? OPTION_SETTING_START : OPTION_SETTING_START + 0x80;
PROJECT_SECURE_OR_FLAT = (!DEFINED(PROJECT_NONSECURE) || DEFINED(PROJECT_SECURE)) && OPTION_SETTING_LENGTH;
USE_OPTION_SETTING_NS = DEFINED(PROJECT_NONSECURE);
OPTION_SETTING_SAS_LENGTH = OPTION_SETTING_LENGTH == 0 ? 0 :
							OPTION_SETTING_LENGTH - 0x34;

MEMORY
{
    OPTION_SETTING (r)      : ORIGIN = OPTION_SETTING_START + NS_IMAGE_OFFSET, LENGTH = OPTION_SETTING_LENGTH
    OPTION_SETTING_OFS (r)  : ORIGIN = OPTION_SETTING_START + NS_IMAGE_OFFSET, LENGTH = 0x18
    OPTION_SETTING_SAS (r)  : ORIGIN = OPTION_SETTING_START + NS_IMAGE_OFFSET + 0x34, LENGTH = OPTION_SETTING_SAS_LENGTH
    OPTION_SETTING_S (r)    : ORIGIN = OPTION_SETTING_S_START + NS_IMAGE_OFFSET, LENGTH = OPTION_SETTING_S_LENGTH
    ID_CODE (rx) 			: ORIGIN = ID_CODE_START, LENGTH = ID_CODE_LENGTH
}

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>

SECTIONS
{
	__tz_ID_CODE_S = ORIGIN(ID_CODE);

    SECTION_PROLOGUE(.id_code,(NOLOAD),)
    {
        KEEP(*(.id_code*))
    } GROUP_LINK_IN(ID_CODE)

	/* Symbol required for RA Configuration tool. */
    __tz_OPTION_SETTING_S = ORIGIN(OPTION_SETTING_OFS);

    SECTION_PROLOGUE(.option_setting_ofs,(NOLOAD),)
    {
        __OPTION_SETTING_OFS_Start = .;
        KEEP(*(.option_setting_ofs0))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_OFS_Start + 0x04 : __OPTION_SETTING_OFS_Start;
        KEEP(*(.option_setting_ofs2))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_OFS_Start + 0x10 : __OPTION_SETTING_OFS_Start;
        KEEP(*(.option_setting_dualsel))
        __OPTION_SETTING_OFS_End = .;
    } GROUP_LINK_IN(OPTION_SETTING_OFS) = 0xFF

	SECTION_PROLOGUE(.option_setting_sas,(NOLOAD),)
    {
        __OPTION_SETTING_SAS_Start = .;
        KEEP(*(.option_setting_sas))
        __OPTION_SETTING_SAS_End = .;
    } GROUP_LINK_IN(OPTION_SETTING_SAS) = 0xFF

    /* Symbol required for RA Configuration tool. */
    __tz_OPTION_SETTING_N = ABSOLUTE(OPTION_SETTING_START_NS);

    SECTION_PROLOGUE(.option_setting_ns,(NOLOAD),)
    {
        __OPTION_SETTING_NS_Start = .;
        KEEP(*(.option_setting_ofs1))
        . = USE_OPTION_SETTING_NS ? __OPTION_SETTING_NS_Start + 0x04 : __OPTION_SETTING_NS_Start;
        KEEP(*(.option_setting_ofs3))
        . = USE_OPTION_SETTING_NS ? __OPTION_SETTING_NS_Start + 0x10 : __OPTION_SETTING_NS_Start;
        KEEP(*(.option_setting_banksel))
        . = USE_OPTION_SETTING_NS ? __OPTION_SETTING_NS_Start + 0x40 : __OPTION_SETTING_NS_Start;
        KEEP(*(.option_setting_bps0))
        . = USE_OPTION_SETTING_NS ? __OPTION_SETTING_NS_Start + 0x44 : __OPTION_SETTING_NS_Start;
        KEEP(*(.option_setting_bps1))
        . = USE_OPTION_SETTING_NS ? __OPTION_SETTING_NS_Start + 0x48 : __OPTION_SETTING_NS_Start;
        KEEP(*(.option_setting_bps2))
        . = USE_OPTION_SETTING_NS ? __OPTION_SETTING_NS_Start + 0x4C : __OPTION_SETTING_NS_Start;
        KEEP(*(.option_setting_bps3))
        . = USE_OPTION_SETTING_NS ? __OPTION_SETTING_NS_Start + 0x60 : __OPTION_SETTING_NS_Start;
        KEEP(*(.option_setting_pbps0))
        . = USE_OPTION_SETTING_NS ? __OPTION_SETTING_NS_Start + 0x64 : __OPTION_SETTING_NS_Start;
        KEEP(*(.option_setting_pbps1))
        . = USE_OPTION_SETTING_NS ? __OPTION_SETTING_NS_Start + 0x68 : __OPTION_SETTING_NS_Start;
        KEEP(*(.option_setting_pbps2))
        . = USE_OPTION_SETTING_NS ? __OPTION_SETTING_NS_Start + 0x6C : __OPTION_SETTING_NS_Start;
        KEEP(*(.option_setting_pbps3))
        __OPTION_SETTING_NS_End = .;
    } GROUP_LINK_IN(OPTION_SETTING) = 0xFF

    /* Symbol required for RA Configuration tool. */
    __tz_OPTION_SETTING_S_S = ORIGIN(OPTION_SETTING_S);

    SECTION_PROLOGUE(.option_setting_s,(NOLOAD),)
    {
        __OPTION_SETTING_S_Start = .;
        KEEP(*(.option_setting_ofs1_sec))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x04 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_ofs3_sec))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x10 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_banksel_sec))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x40 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_bps_sec0))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x44 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_bps_sec1))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x48 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_bps_sec2))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x4C : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_bps_sec3))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x60 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_pbps_sec0))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x64 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_pbps_sec1))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x68 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_pbps_sec2))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x6C : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_pbps_sec3))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x80 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_ofs1_sel))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x84 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_ofs3_sel))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0x90 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_banksel_sel))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0xC0 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_bps_sel0))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0xC4 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_bps_sel1))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0xC8 : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_bps_sel2))
        . = PROJECT_SECURE_OR_FLAT ? __OPTION_SETTING_S_Start + 0xCC : __OPTION_SETTING_S_Start;
        KEEP(*(.option_setting_bps_sel3))
        __OPTION_SETTING_S_End = .;
    } GROUP_LINK_IN(OPTION_SETTING_S) = 0xFF

    /* Symbol required for RA Configuration tool. */
    __tz_OPTION_SETTING_S_N = __OPTION_SETTING_S_End;
}
