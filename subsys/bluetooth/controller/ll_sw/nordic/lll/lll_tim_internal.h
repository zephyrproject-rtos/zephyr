/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TIFS_US 150

/* Macro to return PDU time */
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#define PKT_US(octets, phy) \
	(((phy) & BIT(2)) ? \
	 (80 + 256 + 16 + 24 + ((((2 + (octets) + 4) * 8) + 24 + 3) * 8)) : \
	 (((octets) + 14) * 8 / BIT(((phy) & 0x03) >> 1)))
#else /* !CONFIG_BT_CTLR_PHY_CODED */
#define PKT_US(octets, phy) \
	(((octets) + 14) * 8 / BIT(((phy) & 0x03) >> 1))
#endif /* !CONFIG_BT_CTLR_PHY_CODED */


static inline u32_t addr_us_get(u8_t phy)
{
	switch (phy) {
	default:
	case BIT(0):
		return 40;
	case BIT(1):
		return 24;
	case BIT(2):
		return 376;
	}
}
