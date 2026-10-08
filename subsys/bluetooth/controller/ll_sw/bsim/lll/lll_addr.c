/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/addr.h>

#include "pdu_df.h"
#include "pdu_vendor.h"
#include "pdu.h"

#include "lll_filter.h"
#include "lll_addr.h"

void lll_addr_match(const struct pdu_adv *pdu, const struct lll_filter *filter,
		    struct lll_addr_match *match)
{
	const uint8_t *addr = pdu->payload;

	match->devmatch_ok = 0U;
	match->devmatch_id = FILTER_IDX_NONE;

	if (pdu->len < BDADDR_SIZE) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST) && (filter != NULL)) {
		match->devmatch_ok = ull_filter_lll_fal_match(filter, pdu->tx_addr, addr,
							      &match->devmatch_id);
	}
}
