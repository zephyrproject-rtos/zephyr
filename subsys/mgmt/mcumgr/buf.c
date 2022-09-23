/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>
#include <mgmt/mgmt.h>

NET_BUF_POOL_DEFINE(pkt_pool, CONFIG_MCUMGR_BUF_COUNT, CONFIG_MCUMGR_BUF_SIZE,
		    CONFIG_MCUMGR_BUF_USER_DATA_SIZE, NULL);

struct net_buf *
mcumgr_buf_alloc(void)
{
	return net_buf_alloc(&pkt_pool, K_NO_WAIT);
}

void
mcumgr_buf_free(struct net_buf *nb)
{
	net_buf_unref(nb);
}
