/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_mem_cfg_sec))

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_dualsel))
SECTION_DATA_PROLOGUE(.ofs_dualsel, DT_REG_ADDR(DT_NODELABEL(ofs_dualsel)),)
{
	KEEP(*(.ofs_dualsel))
} GROUP_LINK_IN(OFS_DUALSEL)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_sas))
SECTION_DATA_PROLOGUE(.ofs_sas, DT_REG_ADDR(DT_NODELABEL(ofs_sas)),)
{
	KEEP(*(.ofs_sas))
} GROUP_LINK_IN(OFS_SAS)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_sas2))
SECTION_DATA_PROLOGUE(.ofs_sas2, DT_REG_ADDR(DT_NODELABEL(ofs_sas2)),)
{
	KEEP(*(.ofs_sas2))
} GROUP_LINK_IN(OFS_SAS2)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_banksel_sec))
SECTION_DATA_PROLOGUE(.ofs_banksel_sec, DT_REG_ADDR(DT_NODELABEL(ofs_banksel_sec)),)
{
	KEEP(*(.ofs_banksel_sec))
} GROUP_LINK_IN(OFS_BANKSEL_SEC)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_blsel_sec))
SECTION_DATA_PROLOGUE(.ofs_blsel_sec, DT_REG_ADDR(DT_NODELABEL(ofs_blsel_sec)),)
{
	KEEP(*(.ofs_blsel_sec))
} GROUP_LINK_IN(OFS_BLSEL_SEC)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_banksel_sel))
SECTION_DATA_PROLOGUE(.ofs_banksel_sel, DT_REG_ADDR(DT_NODELABEL(ofs_banksel_sel)),)
{
	KEEP(*(.ofs_banksel_sel))
} GROUP_LINK_IN(OFS_BANKSEL_SEL)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_blsel_sel))
SECTION_DATA_PROLOGUE(.ofs_blsel_sel, DT_REG_ADDR(DT_NODELABEL(ofs_blsel_sel)),)
{
	KEEP(*(.ofs_blsel_sel))
} GROUP_LINK_IN(OFS_BLSEL_SEL)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_bpsdm_sec))
SECTION_DATA_PROLOGUE(.ofs_bpsdm_sec, DT_REG_ADDR(DT_NODELABEL(ofs_bpsdm_sec)),)
{
	KEEP(*(.ofs_bpsdm_sec))
} GROUP_LINK_IN(OFS_BPSDM_SEC)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_bps_sec))
SECTION_DATA_PROLOGUE(.ofs_bps_sec, DT_REG_ADDR(DT_NODELABEL(ofs_bps_sec)),)
{
	KEEP(*(.ofs_bps_sec))
} GROUP_LINK_IN(OFS_BPS_SEC)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_bps_sec_dual))
SECTION_DATA_PROLOGUE(.ofs_bps_sec_dual, DT_REG_ADDR(DT_NODELABEL(ofs_bps_sec_dual)),)
{
	KEEP(*(.ofs_bps_sec_dual))
} GROUP_LINK_IN(OFS_BPS_SEC_DUAL)
#endif

#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_opt_sec))

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_fsblctrl0))
SECTION_DATA_PROLOGUE(.ofs_data_flash_fsblctrl0, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_fsblctrl0)),)
{
	KEEP(*(.ofs_data_flash_fsblctrl0))
} GROUP_LINK_IN(OFS_DATA_FLASH_FSBLCTRL0)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_fsblctrl1))
SECTION_DATA_PROLOGUE(.ofs_data_flash_fsblctrl1, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_fsblctrl1)),)
{
	KEEP(*(.ofs_data_flash_fsblctrl1))
} GROUP_LINK_IN(OFS_DATA_FLASH_FSBLCTRL1)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_fsblctrl2))
SECTION_DATA_PROLOGUE(.ofs_data_flash_fsblctrl2, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_fsblctrl2)),)
{
	KEEP(*(.ofs_data_flash_fsblctrl2))
} GROUP_LINK_IN(OFS_DATA_FLASH_FSBLCTRL2)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_samr))
SECTION_DATA_PROLOGUE(.ofs_data_flash_samr, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_samr)),)
{
	KEEP(*(.ofs_data_flash_samr))
} GROUP_LINK_IN(OFS_DATA_FLASH_SAMR)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_sacc0))
SECTION_DATA_PROLOGUE(.ofs_data_flash_sacc0, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_sacc0)),)
{
	KEEP(*(.ofs_data_flash_sacc0))
} GROUP_LINK_IN(OFS_DATA_FLASH_SACC0)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_sacc10))
SECTION_DATA_PROLOGUE(.ofs_data_flash_sacc10, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_sacc10)),)
{
	KEEP(*(.ofs_data_flash_sacc10))
} GROUP_LINK_IN(OFS_DATA_FLASH_SACC10)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_sacc1))
SECTION_DATA_PROLOGUE(.ofs_data_flash_sacc1, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_sacc1)),)
{
	KEEP(*(.ofs_data_flash_sacc1))
} GROUP_LINK_IN(OFS_DATA_FLASH_SACC1)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_sacc11))
SECTION_DATA_PROLOGUE(.ofs_data_flash_sacc11, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_sacc11)),)
{
	KEEP(*(.ofs_data_flash_sacc11))
} GROUP_LINK_IN(OFS_DATA_FLASH_SACC11)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_sacc2))
SECTION_DATA_PROLOGUE(.ofs_data_flash_sacc2, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_sacc2)),)
{
	KEEP(*(.ofs_data_flash_sacc2))
} GROUP_LINK_IN(OFS_DATA_FLASH_SACC2)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_sacc12))
SECTION_DATA_PROLOGUE(.ofs_data_flash_sacc12, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_sacc12)),)
{
	KEEP(*(.ofs_data_flash_sacc12))
} GROUP_LINK_IN(OFS_DATA_FLASH_SACC12)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_sacc3))
SECTION_DATA_PROLOGUE(.ofs_data_flash_sacc3, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_sacc3)),)
{
	KEEP(*(.ofs_data_flash_sacc3))
} GROUP_LINK_IN(OFS_DATA_FLASH_SACC3)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_data_flash_sacc13))
SECTION_DATA_PROLOGUE(.ofs_data_flash_sacc13, DT_REG_ADDR(DT_NODELABEL(ofs_data_flash_sacc13)),)
{
	KEEP(*(.ofs_data_flash_sacc13))
} GROUP_LINK_IN(OFS_DATA_FLASH_SACC13)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_gpotp))
SECTION_DATA_PROLOGUE(.ofs_gpotp, DT_REG_ADDR(DT_NODELABEL(ofs_gpotp)),)
{
	KEEP(*(.ofs_gpotp))
} GROUP_LINK_IN(OFS_GPOTP)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_ofs0))
SECTION_DATA_PROLOGUE(.ofs_ofs0, DT_REG_ADDR(DT_NODELABEL(ofs_ofs0)),)
{
	KEEP(*(.ofs_ofs0))
} GROUP_LINK_IN(OFS_OFS0)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_ofs2))
SECTION_DATA_PROLOGUE(.ofs_ofs2, DT_REG_ADDR(DT_NODELABEL(ofs_ofs2)),)
{
	KEEP(*(.ofs_ofs2))
} GROUP_LINK_IN(OFS_OFS2)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_mde))
SECTION_DATA_PROLOGUE(.ofs_mde, DT_REG_ADDR(DT_NODELABEL(ofs_mde)),)
{
	KEEP(*(.ofs_mde))
} GROUP_LINK_IN(OFS_MDE)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_ofs1_sec))
SECTION_DATA_PROLOGUE(.ofs_ofs1_sec, DT_REG_ADDR(DT_NODELABEL(ofs_ofs1_sec)),)
{
	KEEP(*(.ofs_ofs1_sec))
} GROUP_LINK_IN(OFS_OFS1_SEC)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_ofs3_sec))
SECTION_DATA_PROLOGUE(.ofs_ofs3_sec, DT_REG_ADDR(DT_NODELABEL(ofs_ofs3_sec)),)
{
	KEEP(*(.ofs_ofs3_sec))
} GROUP_LINK_IN(OFS_OFS3_SEC)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_ofs1_sel))
SECTION_DATA_PROLOGUE(.ofs_ofs1_sel, DT_REG_ADDR(DT_NODELABEL(ofs_ofs1_sel)),)
{
	KEEP(*(.ofs_ofs1_sel))
} GROUP_LINK_IN(OFS_OFS1_SEL)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_ofs3_sel))
SECTION_DATA_PROLOGUE(.ofs_ofs3_sel, DT_REG_ADDR(DT_NODELABEL(ofs_ofs3_sel)),)
{
	KEEP(*(.ofs_ofs3_sel))
} GROUP_LINK_IN(OFS_OFS3_SEL)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_pbps_sec))
SECTION_DATA_PROLOGUE(.ofs_pbps_sec, DT_REG_ADDR(DT_NODELABEL(ofs_pbps_sec)),)
{
	KEEP(*(.ofs_pbps_sec))
} GROUP_LINK_IN(OFS_PBPS_SEC)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_pbps_sec_dual))
SECTION_DATA_PROLOGUE(.ofs_pbps_sec_dual, DT_REG_ADDR(DT_NODELABEL(ofs_pbps_sec_dual)),)
{
	KEEP(*(.ofs_pbps_sec_dual))
} GROUP_LINK_IN(OFS_PBPS_SEC_DUAL)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_pbpsdm_sec))
SECTION_DATA_PROLOGUE(.ofs_pbpsdm_sec, DT_REG_ADDR(DT_NODELABEL(ofs_pbpsdm_sec)),)
{
	KEEP(*(.ofs_pbpsdm_sec))
} GROUP_LINK_IN(OFS_PBPSDM_SEC)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_revoke))
SECTION_DATA_PROLOGUE(.ofs_revoke, DT_REG_ADDR(DT_NODELABEL(ofs_revoke)),)
{
	KEEP(*(.ofs_revoke))
} GROUP_LINK_IN(OFS_REVOKE)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_zhuk))
SECTION_DATA_PROLOGUE(.ofs_zhuk, DT_REG_ADDR(DT_NODELABEL(ofs_zhuk)),)
{
	KEEP(*(.ofs_zhuk))
} GROUP_LINK_IN(OFS_ZHUK)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_arccs))
SECTION_DATA_PROLOGUE(.ofs_arccs, DT_REG_ADDR(DT_NODELABEL(ofs_arccs)),)
{
	KEEP(*(.ofs_arccs))
} GROUP_LINK_IN(OFS_ARCCS)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_ubcsr))
SECTION_DATA_PROLOGUE(.ofs_ubcsr, DT_REG_ADDR(DT_NODELABEL(ofs_ubcsr)),)
{
	KEEP(*(.ofs_ubcsr))
} GROUP_LINK_IN(OFS_UBCSR)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ofs_ofsps))
SECTION_DATA_PROLOGUE(.ofs_ofsps, DT_REG_ADDR(DT_NODELABEL(ofs_ofsps)),)
{
	KEEP(*(.ofs_ofsps))
} GROUP_LINK_IN(OFS_OFSPS)
#endif

#endif
