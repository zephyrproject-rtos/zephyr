/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/platform/hooks.h>
#include <soc.h>

#if defined(CONFIG_ETH_NXP_IMX_NETC) && (DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(netc)) != 0)
#define FRDM_IMXRT1186_HAS_NETC_ENABLED 1
#else
#define FRDM_IMXRT1186_HAS_NETC_ENABLED 0
#endif

#if FRDM_IMXRT1186_HAS_NETC_ENABLED
static void frdm_imxrt1186_netc_configure(void)
{
	/* RGMII mode */
	BLK_CTRL_WAKEUPMIX->NETC_LINK_CFG[0] = BLK_CTRL_WAKEUPMIX_NETC_LINK_CFG_MII_PROT(2);
	BLK_CTRL_WAKEUPMIX->NETC_LINK_CFG[2] = BLK_CTRL_WAKEUPMIX_NETC_LINK_CFG_MII_PROT(2);

	/* Unlock the IERB. It will warm reset whole NETC. */
	NETC_PRIV->NETCRR &= ~NETC_PRIV_NETCRR_LOCK_MASK;
	while ((NETC_PRIV->NETCRR & NETC_PRIV_NETCRR_LOCK_MASK) != 0U) {
	}

	/* Set the access attribute, otherwise MSIX access will be blocked. */
	NETC_IERB->ARRAY_NUM_RC[0].RCMSIAMQR &= ~(7U << 27);
	NETC_IERB->ARRAY_NUM_RC[0].RCMSIAMQR |= (1U << 27);

	/* Lock the IERB. */
	NETC_PRIV->NETCRR |= NETC_PRIV_NETCRR_LOCK_MASK;
	while ((NETC_PRIV->NETCSR & NETC_PRIV_NETCSR_STATE_MASK) != 0U) {
	}
}
#endif /* FRDM_IMXRT1186_HAS_NETC_ENABLED */

void board_early_init_hook(void)
{
#if FRDM_IMXRT1186_HAS_NETC_ENABLED
	frdm_imxrt1186_netc_configure();
#endif
}
