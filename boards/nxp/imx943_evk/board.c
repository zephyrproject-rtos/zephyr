/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

static int board_init(void)
{
#if defined(CONFIG_ETH_NXP_IMX_NETC) && (DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(netc)) != 0) \
	&& !defined(CONFIG_CPU_CORTEX_A)
	/*
	 * PCS(Physical Coding Sublayer) protocols on link0-5,
	 * xxxx xxxx xxxx xxx1: 1G SGMII
	 * xxxx xxxx xxxx xx1x: OC-SGMII(i.e.: OverClock 2.5 G SGMII)
	 */
	BLK_CTRL_NETCMIX->CFG_LINK_PCS_PROT_0 |=
		BLK_CTRL_NETCMIX_CFG_LINK_PCS_PROT_0_CFG_LINK_PCS_PROT_0(2U);
	BLK_CTRL_NETCMIX->CFG_LINK_PCS_PROT_1 |=
		BLK_CTRL_NETCMIX_CFG_LINK_PCS_PROT_1_CFG_LINK_PCS_PROT_1(2U);
	BLK_CTRL_NETCMIX->CFG_LINK_PCS_PROT_2 |=
		BLK_CTRL_NETCMIX_CFG_LINK_PCS_PROT_2_CFG_LINK_PCS_PROT_2(1U);
	BLK_CTRL_NETCMIX->CFG_LINK_PCS_PROT_3 |=
		BLK_CTRL_NETCMIX_CFG_LINK_PCS_PROT_3_CFG_LINK_PCS_PROT_3(1U);
	BLK_CTRL_NETCMIX->CFG_LINK_PCS_PROT_4 |=
		BLK_CTRL_NETCMIX_CFG_LINK_PCS_PROT_4_CFG_LINK_PCS_PROT_4(1U);
	BLK_CTRL_NETCMIX->CFG_LINK_PCS_PROT_5 |=
		BLK_CTRL_NETCMIX_CFG_LINK_PCS_PROT_5_CFG_LINK_PCS_PROT_5(1U);

	/*
	 * MII protocol for port0~5
	 * 0b0000..MII
	 * 0b0001..RMII
	 * 0b0010..RGMII
	 * 0b0011..SGMII
	 * 0b0100~0b1111..Reserved
	 */
	BLK_CTRL_NETCMIX->NETC_LINK_CFG0 |=
		BLK_CTRL_NETCMIX_NETC_LINK_CFG0_MII_PROT(0x0U);
	BLK_CTRL_NETCMIX->NETC_LINK_CFG1 |=
		BLK_CTRL_NETCMIX_NETC_LINK_CFG1_MII_PROT(0x0U);
	BLK_CTRL_NETCMIX->NETC_LINK_CFG2 |=
		BLK_CTRL_NETCMIX_NETC_LINK_CFG2_MII_PROT(0x2U);
	BLK_CTRL_NETCMIX->NETC_LINK_CFG3 |=
		BLK_CTRL_NETCMIX_NETC_LINK_CFG3_MII_PROT(0x2U);
	BLK_CTRL_NETCMIX->NETC_LINK_CFG4 |=
		BLK_CTRL_NETCMIX_NETC_LINK_CFG4_MII_PROT(0x2U);
	BLK_CTRL_NETCMIX->NETC_LINK_CFG5 |=
		BLK_CTRL_NETCMIX_NETC_LINK_CFG5_MII_PROT(0x2U);

	/*
	 * ETH2 selection: MAC2(switch port2) or MAC3(enetc0)
	 * 0b - MAC2 selected
	 * 1b - MAC3 selected
	 */
	BLK_CTRL_NETCMIX->EXT_PIN_CONTROL |= BLK_CTRL_NETCMIX_EXT_PIN_CONTROL_mac2_mac3_sel(1U);

	/* Unlock the IERB. It will warm reset whole NETC. */
	NETC_PRIV->NETCRR &= ~NETC_PRIV_NETCRR_LOCK_MASK;
	while ((NETC_PRIV->NETCRR & NETC_PRIV_NETCRR_LOCK_MASK) != 0U) {
	}

	/* Lock the IERB. */
	NETC_PRIV->NETCRR |= NETC_PRIV_NETCRR_LOCK_MASK;
	while ((NETC_PRIV->NETCSR & NETC_PRIV_NETCSR_STATE_MASK) != 0U) {
	}
#endif
	return 0;
}

/*
 * Because platform is using ARM SCMI, drivers like scmi, mbox etc. are
 * initialized during PRE_KERNEL_1. Common init hooks is not able to use.
 * SoC early init and board early init could be run during PRE_KERNEL_2 instead.
 */
SYS_INIT(board_init, PRE_KERNEL_2, 10);
