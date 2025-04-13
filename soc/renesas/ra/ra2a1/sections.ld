/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(id_code))

SECTION_PROLOGUE(.id_code,,)
{
	KEEP(*(.id_code*))
} GROUP_LINK_IN(ID_CODE)

#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs))

SECTION_PROLOGUE(.option_setting_ofs,,)
{
	__OPTION_SETTING_OFS_Start = .;
	KEEP(*(.option_setting_ofs0))
	. = __OPTION_SETTING_OFS_Start + 0x04;
	KEEP(*(.option_setting_ofs2))
	. = __OPTION_SETTING_OFS_Start + 0x10;
	KEEP(*(.option_setting_dualsel))
	__OPTION_SETTING_OFS_End = .;
} GROUP_LINK_IN(OPTION_SETTING_OFS) = 0xFF

#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_sas))

SECTION_PROLOGUE(.option_setting_sas,,)
{
	__OPTION_SETTING_SAS_Start = .;
	KEEP(*(.option_setting_sas))
	__OPTION_SETTING_SAS_End = .;
} GROUP_LINK_IN(OPTION_SETTING_SAS) = 0xFF

#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ns))

SECTION_PROLOGUE(.option_setting_ns,,)
{
	__OPTION_SETTING_NS_Start = .;
	KEEP(*(.option_setting_ofs1))
        . = __OPTION_SETTING_NS_Start + 0x04;
	KEEP(*(.option_setting_ofs3))
        . = __OPTION_SETTING_NS_Start + 0x10;
	KEEP(*(.option_setting_banksel))
        . = __OPTION_SETTING_NS_Start + 0x40;
	KEEP(*(.option_setting_bps0))
        . = __OPTION_SETTING_NS_Start + 0x44;
	KEEP(*(.option_setting_bps1))
        . = __OPTION_SETTING_NS_Start + 0x48;
	KEEP(*(.option_setting_bps2))
        . = __OPTION_SETTING_NS_Start + 0x4C;
	KEEP(*(.option_setting_bps3))
        . = __OPTION_SETTING_NS_Start + 0x60;
	KEEP(*(.option_setting_pbps0))
        . = __OPTION_SETTING_NS_Start + 0x64;
	KEEP(*(.option_setting_pbps1))
        . = __OPTION_SETTING_NS_Start + 0x68;
	KEEP(*(.option_setting_pbps2))
        . = __OPTION_SETTING_NS_Start + 0x6C;
	KEEP(*(.option_setting_pbps3))
	__OPTION_SETTING_NS_End = .;
} GROUP_LINK_IN(OPTION_SETTING) = 0xFF

#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_s))

SECTION_PROLOGUE(.option_setting_s,,)
{
	__OPTION_SETTING_S_Start = .;
	KEEP(*(.option_setting_ofs1_sec))
	. = __OPTION_SETTING_S_Start + 0x04;
	KEEP(*(.option_setting_ofs3_sec))
	. = __OPTION_SETTING_S_Start + 0x10;
	KEEP(*(.option_setting_banksel_sec))
	. = __OPTION_SETTING_S_Start + 0x40;
	KEEP(*(.option_setting_bps_sec0))
	. = __OPTION_SETTING_S_Start + 0x44;
	KEEP(*(.option_setting_bps_sec1))
	. = __OPTION_SETTING_S_Start + 0x48;
	KEEP(*(.option_setting_bps_sec2))
	. = __OPTION_SETTING_S_Start + 0x4C;
	KEEP(*(.option_setting_bps_sec3))
	. = __OPTION_SETTING_S_Start + 0x60;
	KEEP(*(.option_setting_pbps_sec0))
	. = __OPTION_SETTING_S_Start + 0x64;
	KEEP(*(.option_setting_pbps_sec1))
	. = __OPTION_SETTING_S_Start + 0x68;
	KEEP(*(.option_setting_pbps_sec2))
	. = __OPTION_SETTING_S_Start + 0x6C;
	KEEP(*(.option_setting_pbps_sec3))
	. = __OPTION_SETTING_S_Start + 0x80;
	KEEP(*(.option_setting_ofs1_sel))
	. = __OPTION_SETTING_S_Start + 0x84;
	KEEP(*(.option_setting_ofs3_sel))
	. = __OPTION_SETTING_S_Start + 0x90;
	KEEP(*(.option_setting_banksel_sel))
	. = __OPTION_SETTING_S_Start + 0xC0;
	KEEP(*(.option_setting_bps_sel0))
	. = __OPTION_SETTING_S_Start + 0xC4;
	KEEP(*(.option_setting_bps_sel1))
	. = __OPTION_SETTING_S_Start + 0xC8;
	KEEP(*(.option_setting_bps_sel2))
	. = __OPTION_SETTING_S_Start + 0xCC;
	KEEP(*(.option_setting_bps_sel3))
	__OPTION_SETTING_S_End = .;
} GROUP_LINK_IN(OPTION_SETTING_S) = 0xFF

#endif
