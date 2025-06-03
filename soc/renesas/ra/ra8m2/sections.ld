/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs0))
SECTION_DATA_PROLOGUE(.option_setting_ofs0, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs0)),)
{
	KEEP(*(.option_setting_ofs0))
} GROUP_LINK_IN(OFS_OFS0_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs2))
SECTION_DATA_PROLOGUE(.option_setting_ofs2, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs2)),)
{
	KEEP(*(.option_setting_ofs2))
} GROUP_LINK_IN(OFS_OFS2_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_sas))
SECTION_DATA_PROLOGUE(.option_setting_sas, DT_REG_ADDR(DT_NODELABEL(option_setting_sas)),)
{
	KEEP(*(.option_setting_sas))
} GROUP_LINK_IN(OFS_SAS_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs1_sec))
SECTION_DATA_PROLOGUE(.option_setting_ofs1_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs1_sec)),)
{
	KEEP(*(.option_setting_ofs1_sec))
} GROUP_LINK_IN(OFS_OFS1_SEC_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs3_sec))
SECTION_DATA_PROLOGUE(.option_setting_ofs3_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs3_sec)),)
{
	KEEP(*(.option_setting_ofs3_sec))
} GROUP_LINK_IN(OFS_OFS3_SEC_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs1_sel))
SECTION_DATA_PROLOGUE(.option_setting_ofs1_sel, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs1_sel)),)
{
	KEEP(*(.option_setting_ofs1_sel))
} GROUP_LINK_IN(OFS_OFS1_SEL_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs3_sel))
SECTION_DATA_PROLOGUE(.option_setting_ofs3_sel, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs3_sel)),)
{
	KEEP(*(.option_setting_ofs3_sel))
} GROUP_LINK_IN(OFS_OFS3_SEL_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_bps_sec))
SECTION_DATA_PROLOGUE(.option_setting_bps_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_bps_sec)),)
{
	KEEP(*(.option_setting_bps_sec))
} GROUP_LINK_IN(OFS_BPS_SEC_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_otp_pbps_sec))
SECTION_DATA_PROLOGUE(.option_setting_otp_pbps_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_otp_pbps_sec)),)
{
	KEEP(*(.option_setting_otp_pbps_sec))
} GROUP_LINK_IN(OFS_OTP_PBPS_SEC_MEMORY)
#endif
