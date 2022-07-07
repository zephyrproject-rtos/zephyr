/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/buf.h>
#include <mgmt/mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

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

void
cbor_nb_reader_init(struct cbor_nb_reader *cnr,
		    struct net_buf *nb)
{
	/* Skip the mgmt_hdr */
	void *new_ptr = net_buf_pull(nb, sizeof(struct mgmt_hdr));

	cnr->nb = nb;
	zcbor_new_decode_state(cnr->zs, ARRAY_SIZE(cnr->zs), new_ptr,
			       cnr->nb->len, 1);
}

void
cbor_nb_writer_init(struct cbor_nb_writer *cnw, struct net_buf *nb)
{
	net_buf_reset(nb);
	cnw->nb = nb;
	cnw->nb->len = sizeof(struct mgmt_hdr);
	zcbor_new_encode_state(cnw->zs, 2, nb->data + sizeof(struct mgmt_hdr),
			       net_buf_tailroom(nb), 0);
}
