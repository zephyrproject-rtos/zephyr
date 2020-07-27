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

struct ppp_parse_option_array_data {
	int idx;
	struct ppp_option_pkt *options;
	int options_len;
};

static int ppp_parse_option_array_elem(struct net_pkt *pkt, uint8_t code,
				       uint8_t len, void *user_data)
{
	struct ppp_parse_option_array_data *parse_data = user_data;
	struct ppp_option_pkt *options = parse_data->options;
	int options_len = parse_data->options_len;
	int idx = parse_data->idx;

	if (idx >= options_len) {
		NET_DBG("Cannot insert options (max %d)", options_len);
		return -ENOMEM;
	}

	options[idx].type.lcp = code;
	options[idx].len = len;

	if (len > 0) {
		/* There is an option value here */
		net_pkt_cursor_backup(pkt, &options[idx].value);
	}

	parse_data->idx++;

	return 0;
}

int ppp_parse_options_array(struct ppp_fsm *fsm, struct net_pkt *pkt,
			    uint16_t length, struct ppp_option_pkt options[],
			    int options_len)
{
	struct ppp_parse_option_array_data parse_data = {
		.options = options,
		.options_len = options_len,
	};

	return ppp_parse_options(fsm, pkt, length, ppp_parse_option_array_elem,
				 &parse_data);
}

int ppp_parse_options(struct ppp_fsm *fsm, struct net_pkt *pkt,
		      uint16_t length,
		      int (*parse)(struct net_pkt *pkt, uint8_t code,
				   uint8_t len, void *user_data),
		      void *user_data)
{
	int remaining = length, pkt_remaining;
	uint8_t opt_type, opt_len, opt_val_len;
	int ret;
	struct net_pkt_cursor cursor;

	pkt_remaining = net_pkt_remaining_data(pkt);
	if (remaining != pkt_remaining) {
		NET_DBG("Expecting %d but pkt data length is %d bytes",
			remaining, pkt_remaining);
		return -EMSGSIZE;
	}

	while (remaining > 0) {
		ret = net_pkt_read_u8(pkt, &opt_type);
		if (ret < 0) {
			NET_DBG("Cannot read %s (%d) (remaining len %d)",
				"opt_type", ret, pkt_remaining);
			return -EBADMSG;
		}

		ret = net_pkt_read_u8(pkt, &opt_len);
		if (ret < 0) {
			NET_DBG("Cannot read %s (%d) (remaining len %d)",
				"opt_len", ret, remaining);
			return -EBADMSG;
		}

		opt_val_len = opt_len - sizeof(opt_type) - sizeof(opt_len);

		net_pkt_cursor_backup(pkt, &cursor);

		NET_DBG("[%s/%p] option %s (%d) len %d", fsm->name, fsm,
			ppp_option2str(fsm->protocol, opt_type), opt_type,
			opt_len);

		ret = parse(pkt, opt_type, opt_val_len, user_data);
		if (ret < 0) {
			return ret;
		}

		net_pkt_cursor_restore(pkt, &cursor);

		net_pkt_skip(pkt, opt_val_len);
		remaining -= opt_len;
	}

	if (remaining < 0) {
		return -EBADMSG;
	}

	return 0;
}
