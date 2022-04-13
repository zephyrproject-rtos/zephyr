/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/buf.h>
#include "mgmt/mgmt.h"
#include "smp/smp.h"
#include <zephyr/mgmt/mcumgr/smp.h>
#include "smp_reassembly.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcumgr_smp, CONFIG_MCUMGR_SMP_LOG_LEVEL);

/* To be able to unit test some callers some functions need to be
 * demoted to allow overriding them.
 */
#ifdef CONFIG_ZTEST
#define WEAK __weak
#else
#define WEAK
#endif

static const struct mgmt_streamer_cfg zephyr_smp_cbor_cfg;

static void *
zephyr_smp_alloc_rsp(const void *req, void *arg)
{
	const struct net_buf_pool *pool;
	const struct net_buf *req_nb;
	struct net_buf *rsp_nb;
	struct zephyr_smp_transport *zst = arg;

	req_nb = req;

	rsp_nb = mcumgr_buf_alloc();
	if (rsp_nb == NULL) {
		return NULL;
	}

	if (zst->zst_ud_copy) {
		zst->zst_ud_copy(rsp_nb, req_nb);
	} else {
		pool = net_buf_pool_get(req_nb->pool_id);
		memcpy(net_buf_user_data(rsp_nb),
		       net_buf_user_data((void *)req_nb),
		       req_nb->user_data_size);
	}

	return rsp_nb;
}

/**
 * Splits an appropriately-sized fragment from the front of a net_buf, as
 * needed.  If the length of the net_buf is greater than specified maximum
 * fragment size, a new net_buf is allocated, and data is moved from the source
 * net_buf to the new net_buf.  If the net_buf is small enough to fit in a
 * single fragment, the source net_buf is returned unmodified, and the supplied
 * pointer is set to NULL.
 *
 * This function is expected to be called in a loop until the entire source
 * net_buf has been consumed.  For example:
 *
 *     struct net_buf *frag;
 *     struct net_buf *rsp;
 *     ...
 *     while (rsp != NULL) {
 *         frag = zephyr_smp_split_frag(&rsp, zst, get_mtu());
 *         if (frag == NULL) {
 *             net_buf_unref(nb);
 *             return SYS_ENOMEM;
 *         }
 *         send_packet(frag)
 *     }
 *
 * @param nb                    The packet to fragment.  Upon fragmentation,
 *                                  this net_buf is adjusted such that the
 *                                  fragment data is removed.  If the packet
 *                                  constitutes a single fragment, this gets
 *                                  set to NULL on success.
 * @param arg                   The zephyr SMP transport pointer.
 * @param mtu                   The maximum payload size of a fragment.
 *
 * @return                      The next fragment to send on success;
 *                              NULL on failure.
 */
static struct net_buf *
zephyr_smp_split_frag(struct net_buf **nb, void *arg, uint16_t mtu)
{
	struct net_buf *frag;
	struct net_buf *src;

	src = *nb;

	if (src->len <= mtu) {
		*nb = NULL;
		frag = src;
	} else {
		frag = zephyr_smp_alloc_rsp(src, arg);
		if (!frag) {
			return NULL;
		}

		/* Copy fragment payload into new buffer. */
		net_buf_add_mem(frag, src->data, mtu);

		/* Remove fragment from total response. */
		net_buf_pull(src, mtu);
	}

	return frag;
}

static int
zephyr_smp_write_hdr(struct cbor_nb_writer *cnw, const struct mgmt_hdr *hdr)
{
	memcpy(cnw->nb->data, hdr, sizeof(*hdr));

	return 0;
}

static void
zephyr_smp_free_buf(void *buf, void *arg)
{
	struct zephyr_smp_transport *zst = arg;

	if (!buf) {
		return;
	}

	if (zst->zst_ud_free) {
		zst->zst_ud_free(net_buf_user_data((struct net_buf *)buf));
	}

	mcumgr_buf_free(buf);
}

static int
zephyr_smp_tx_rsp(struct smp_streamer *ns, void *rsp, void *arg)
{
	struct zephyr_smp_transport *zst;
	struct net_buf *frag;
	struct net_buf *nb;
	uint16_t mtu;
	int rc;
	int i;

	zst = arg;
	nb = rsp;

	mtu = zst->zst_get_mtu(rsp);
	if (mtu == 0U) {
		/* The transport cannot support a transmission right now. */
		return MGMT_ERR_EUNKNOWN;
	}

	i = 0;
	while (nb != NULL) {
		frag = zephyr_smp_split_frag(&nb, zst, mtu);
		if (frag == NULL) {
			zephyr_smp_free_buf(nb, zst);
			return MGMT_ERR_ENOMEM;
		}

		rc = zst->zst_output(zst, frag);
		if (rc != 0) {
			return MGMT_ERR_EUNKNOWN;
		}
	}

	return 0;
}

/**
 * Processes a single SMP packet and sends the corresponding response(s).
 */
static int
zephyr_smp_process_packet(struct zephyr_smp_transport *zst,
			  struct net_buf *nb)
{
	struct cbor_nb_reader reader;
	struct cbor_nb_writer writer;
	struct smp_streamer streamer;
	int rc;

	streamer = (struct smp_streamer) {
		.mgmt_stmr = {
			.cfg = &zephyr_smp_cbor_cfg,
			.reader = &reader,
			.writer = &writer,
			.cb_arg = zst,
		},
		.tx_rsp_cb = zephyr_smp_tx_rsp,
	};

	rc = smp_process_request_packet(&streamer, nb);
	return rc;
}

/**
 * Processes all received SNP request packets.
 */
static void
zephyr_smp_handle_reqs(struct k_work *work)
{
	struct zephyr_smp_transport *zst;
	struct net_buf *nb;

	zst = (void *)work;

	while ((nb = net_buf_get(&zst->zst_fifo, K_NO_WAIT)) != NULL) {
		zephyr_smp_process_packet(zst, nb);
	}
}

static const struct mgmt_streamer_cfg zephyr_smp_cbor_cfg = {
	.alloc_rsp = zephyr_smp_alloc_rsp,
	.write_hdr = zephyr_smp_write_hdr,
	.free_buf = zephyr_smp_free_buf,
};

void
zephyr_smp_transport_init(struct zephyr_smp_transport *zst,
			  zephyr_smp_transport_out_fn *output_func,
			  zephyr_smp_transport_get_mtu_fn *get_mtu_func,
			  zephyr_smp_transport_ud_copy_fn *ud_copy_func,
			  zephyr_smp_transport_ud_free_fn *ud_free_func)
{
	*zst = (struct zephyr_smp_transport) {
		.zst_output = output_func,
		.zst_get_mtu = get_mtu_func,
		.zst_ud_copy = ud_copy_func,
		.zst_ud_free = ud_free_func,
	};

#ifdef CONFIG_MCUMGR_SMP_REASSEMBLY
	zephyr_smp_reassembly_init(zst);
#endif

	k_work_init(&zst->zst_work, zephyr_smp_handle_reqs);
	k_fifo_init(&zst->zst_fifo);
}

WEAK void
zephyr_smp_rx_req(struct zephyr_smp_transport *zst, struct net_buf *nb)
{
	net_buf_put(&zst->zst_fifo, nb);
	k_work_submit(&zst->zst_work);
}
