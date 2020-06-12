/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IEEE802154_DW1000_PRIV_H
#define IEEE802154_DW1000_PRIV_H

#include <stdint.h>
#include <stdbool.h>

struct dwt_phy_config {
	uint8_t channel;	/* Channel 1, 2, 3, 4, 5, 7 */
	uint8_t dr;	/* Data rate DWT_BR_110K, DWT_BR_850K, DWT_BR_6M8 */
	uint8_t prf;	/* PRF DWT_PRF_16M or DWT_PRF_64M */

	uint8_t rx_pac_l;		/* DWT_PAC8..DWT_PAC64 */
	uint8_t rx_shr_code;	/* RX SHR preamble code */
	uint8_t rx_ns_sfd;		/* non-standard SFD */
	uint16_t rx_sfd_to;	/* SFD timeout value (in symbols)
				 * (tx_shr_nsync + 1 + SFD_length - rx_pac_l)
				 */

	uint8_t tx_shr_code;	/* TX SHR preamble code */
	uint32_t tx_shr_nsync;	/* PLEN index, e.g. DWT_PLEN_64 */

	bool phr_mode_ext;	/* Extended PHR mode: long frame 1024 Byte */
	bool smart_power_en;	/* Enable/Disable smart power */

	float t_shr;
	float t_phr;
	float t_dsym;
};

#endif /* IEEE802154_DW1000_PRIV_H */
