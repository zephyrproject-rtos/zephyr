/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs_conf_sec))

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs0))
SECTION_DATA_PROLOGUE(.option_setting_ofs0, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs0)),)
{
	KEEP(*(.option_setting_ofs0))
} GROUP_LINK_IN(OFS_CONF_SEC_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs2))
SECTION_DATA_PROLOGUE(.option_setting_ofs2, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs2)),)
{
	KEEP(*(.option_setting_ofs2))
} GROUP_LINK_IN(OFS_CONF_SEC_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_sas))
SECTION_DATA_PROLOGUE(.option_setting_sas, DT_REG_ADDR(DT_NODELABEL(option_setting_sas)),)
{
	KEEP(*(.option_setting_sas))
} GROUP_LINK_IN(OFS_CONF_SEC_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs1_sec))
SECTION_DATA_PROLOGUE(.option_setting_ofs1_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs1_sec)),)
{
	KEEP(*(.option_setting_ofs1_sec))
} GROUP_LINK_IN(OFS_CONF_SEC_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs3_sec))
SECTION_DATA_PROLOGUE(.option_setting_ofs3_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs3_sec)),)
{
	KEEP(*(.option_setting_ofs3_sec))
} GROUP_LINK_IN(OFS_CONF_SEC_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs1_sec))
SECTION_DATA_PROLOGUE(.option_setting_ofs1_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs1_sec)),)
{
	KEEP(*(.option_setting_ofs1_sec))
} GROUP_LINK_IN(OFS_CONF_SEC_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs3_sec))
SECTION_DATA_PROLOGUE(.option_setting_ofs3_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs3_sec)),)
{
	KEEP(*(.option_setting_ofs3_sec))
} GROUP_LINK_IN(OFS_CONF_SEC_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_bps_sec))
SECTION_DATA_PROLOGUE(.option_setting_bps_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_bps_sec)),)
{
	KEEP(*(.option_setting_bps_sec))
} GROUP_LINK_IN(OFS_CONF_SEC_MEMORY)
#endif

#endif /* option_setting_ofs_conf_sec */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs_otp_sec))

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_otp_pbps_sec))
SECTION_DATA_PROLOGUE(.option_setting_otp_pbps_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_otp_pbps_sec)),)
{
	KEEP(*(.option_setting_otp_pbps_sec))
} GROUP_LINK_IN(OFS_OTP_SEC_MEMORY)
#endif

#endif /* option_setting_ofs_conf_sec */
