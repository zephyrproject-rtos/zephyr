/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs_conf))

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs0))
SECTION_DATA_PROLOGUE(.option_setting_ofs0, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs0)),)
{
	KEEP(*(.option_setting_ofs0))
} GROUP_LINK_IN(OFS_CONF_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_dualsel))
SECTION_DATA_PROLOGUE(.option_setting_dualsel, DT_REG_ADDR(DT_NODELABEL(option_setting_dualsel)),)
{
	KEEP(*(.option_setting_dualsel))
} GROUP_LINK_IN(OFS_CONF_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs1_sec))
SECTION_DATA_PROLOGUE(.option_setting_ofs1_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs1_sec)),)
{
	KEEP(*(.option_setting_ofs1_sec))
} GROUP_LINK_IN(OFS_CONF_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_banksel_sec))
SECTION_DATA_PROLOGUE(.option_setting_banksel_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_banksel_sec)),)
{
	KEEP(*(.option_setting_banksel_sec))
} GROUP_LINK_IN(OFS_CONF_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_bps_sec))
SECTION_DATA_PROLOGUE(.option_setting_bps_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_bps_sec)),)
{
	KEEP(*(.option_setting_bps_sec))
} GROUP_LINK_IN(OFS_CONF_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_pbps_sec))
SECTION_DATA_PROLOGUE(.option_setting_pbps_sec, DT_REG_ADDR(DT_NODELABEL(option_setting_pbps_sec)),)
{
	KEEP(*(.option_setting_pbps_sec))
} GROUP_LINK_IN(OFS_CONF_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_ofs1_sel))
SECTION_DATA_PROLOGUE(.option_setting_ofs1_sel, DT_REG_ADDR(DT_NODELABEL(option_setting_ofs1_sel)),)
{
	KEEP(*(.option_setting_ofs1_sel))
} GROUP_LINK_IN(OFS_CONF_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_banksel_sel))
SECTION_DATA_PROLOGUE(.option_setting_banksel_sel, DT_REG_ADDR(DT_NODELABEL(option_setting_banksel_sel)),)
{
	KEEP(*(.option_setting_banksel_sel))
} GROUP_LINK_IN(OFS_CONF_MEMORY)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(option_setting_bps_sel))
SECTION_DATA_PROLOGUE(.option_setting_bps_sel, DT_REG_ADDR(DT_NODELABEL(option_setting_bps_sel)),)
{
	KEEP(*(.option_setting_bps_sel))
} GROUP_LINK_IN(OFS_CONF_MEMORY)
#endif

#endif /* option_setting_ofs_conf */
