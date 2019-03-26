/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_pkt.h>

#include <net/ppp.h>

#include "net_private.h"

#include "ppp_internal.h"

#define ALLOC_TIMEOUT K_MSEC(100)

enum net_verdict ppp_parse_options(struct ppp_fsm *fsm,
				   struct net_pkt *pkt,
				   u16_t length,
				   struct ppp_option_pkt options[],
				   int options_len)
{
	int remaining = length, pkt_remaining;
	enum net_verdict verdict;
	u8_t opt_type, opt_len;
	int ret, idx = 0;

	pkt_remaining = net_pkt_remaining_data(pkt);
	if (remaining != pkt_remaining) {
		NET_DBG("Expecting %d but pkt data length is %d bytes",
			remaining, pkt_remaining);
		verdict = NET_DROP;
		goto out;
	}

	while (remaining > 0) {
		ret = net_pkt_read_u8(pkt, &opt_type);
		if (ret < 0) {
			NET_DBG("Cannot read %s (%d) (remaining len %d)",
				"opt_type", ret, pkt_remaining);
			verdict = NET_DROP;
			goto out;
		}

		ret = net_pkt_read_u8(pkt, &opt_len);
		if (ret < 0) {
			NET_DBG("Cannot read %s (%d) (remaining len %d)",
				"opt_len", ret, remaining);
			verdict = NET_DROP;
			goto out;
		}

		if (idx >= options_len) {
			NET_DBG("Cannot insert options (max %d)", options_len);
			verdict = NET_DROP;
			goto out;
		}

		options[idx].type.lcp = opt_type;
		options[idx].len = opt_len;

		NET_DBG("[%s/%p] %s option %s (%d) len %d", fsm->name, fsm,
			"Recv", ppp_option2str(fsm->protocol, opt_type),
			opt_type, opt_len);

		if (opt_len > 2) {
			/* There is an option value here */
			net_pkt_cursor_backup(pkt, &options[idx].value);
		}

		net_pkt_skip(pkt,
			     opt_len - sizeof(opt_type) - sizeof(opt_len));
		remaining -= opt_len;

		idx++;
	};

	if (remaining < 0) {
		verdict = NET_DROP;
		goto out;
	}

	verdict = NET_OK;

out:
	return verdict;
}

struct net_buf *ppp_get_net_buf(struct net_buf *root_buf, u8_t len)
{
	struct net_buf *tmp;

	if (root_buf) {
		tmp = net_buf_frag_last(root_buf);

		if (len > net_buf_tailroom(tmp)) {
			tmp = net_pkt_get_reserve_tx_data(ALLOC_TIMEOUT);
			if (tmp) {
				net_buf_frag_add(root_buf, tmp);
			}
		}

		return tmp;

	}

	tmp = net_pkt_get_reserve_tx_data(ALLOC_TIMEOUT);
	if (tmp) {
		return tmp;
	}

	return NULL;
}
