/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include "net/buf.h"
#include "mgmt/mgmt.h"
#include "mgmt/mcumgr/buf.h"
#include "smp/smp.h"
#include "mgmt/mcumgr/smp.h"

static mgmt_alloc_rsp_fn zephyr_smp_alloc_rsp;
static mgmt_trim_front_fn zephyr_smp_trim_front;
static mgmt_reset_buf_fn zephyr_smp_reset_buf;
static mgmt_write_at_fn zephyr_smp_write_at;
static mgmt_init_reader_fn zephyr_smp_init_reader;
static mgmt_init_writer_fn zephyr_smp_init_writer;
static mgmt_free_buf_fn zephyr_smp_free_buf;
static smp_tx_rsp_fn zephyr_smp_tx_rsp;

static const struct mgmt_streamer_cfg zephyr_smp_cbor_cfg = {
	.alloc_rsp = zephyr_smp_alloc_rsp,
	.trim_front = zephyr_smp_trim_front,
	.reset_buf = zephyr_smp_reset_buf,
	.write_at = zephyr_smp_write_at,
	.init_reader = zephyr_smp_init_reader,
	.init_writer = zephyr_smp_init_writer,
	.free_buf = zephyr_smp_free_buf,
};

void *
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
		       sizeof(req_nb->user_data));
	}

	return rsp_nb;
}

static void
zephyr_smp_trim_front(void *buf, size_t len, void *arg)
{
	struct net_buf *nb;

	nb = buf;
	if (len > nb->len) {
		len = nb->len;
	}

	net_buf_pull(nb, len);
}

/**
 * Splits an appropriately-sized fragment from the front of a net_buf, as
 * neeeded.  If the length of the net_buf is greater than specified maximum
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

		/* Copy fragment payload into new buffer. */
		net_buf_add_mem(frag, src->data, mtu);

		/* Remove fragment from total response. */
		zephyr_smp_trim_front(src, mtu, NULL);
	}

	return frag;
}

static void
zephyr_smp_reset_buf(void *buf, void *arg)
{
	net_buf_reset(buf);
}

static int
zephyr_smp_write_at(struct cbor_encoder_writer *writer, size_t offset,
		    const void *data, size_t len, void *arg)
{
	struct cbor_nb_writer *czw;
	struct net_buf *nb;

	czw = (struct cbor_nb_writer *)writer;
	nb = czw->nb;

	if (offset > nb->len) {
		return MGMT_ERR_EINVAL;
	}

	if ((offset + len) > (nb->size - net_buf_headroom(nb))) {
		return MGMT_ERR_EINVAL;
	}

	memcpy(nb->data + offset, data, len);
	if (nb->len < offset + len) {
		nb->len = offset + len;
		writer->bytes_written = nb->len;
	}

	return 0;
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
			return MGMT_ERR_ENOMEM;
		}

		rc = zst->zst_output(zst, frag);
		if (rc != 0) {
			return MGMT_ERR_EUNKNOWN;
		}
	}

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
zephyr_smp_init_reader(struct cbor_decoder_reader *reader, void *buf,
		       void *arg)
{
	struct cbor_nb_reader *czr;

	czr = (struct cbor_nb_reader *)reader;
	cbor_nb_reader_init(czr, buf);

	return 0;
}

static int
zephyr_smp_init_writer(struct cbor_encoder_writer *writer, void *buf,
		       void *arg)
{
	struct cbor_nb_writer *czw;

	czw = (struct cbor_nb_writer *)writer;
	cbor_nb_writer_init(czw, buf);

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
			.reader = &reader.r,
			.writer = &writer.enc,
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

	while ((nb = k_fifo_get(&zst->zst_fifo, K_NO_WAIT)) != NULL) {
		zephyr_smp_process_packet(zst, nb);
	}
}

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

	k_work_init(&zst->zst_work, zephyr_smp_handle_reqs);
	k_fifo_init(&zst->zst_fifo);
}

void
zephyr_smp_rx_req(struct zephyr_smp_transport *zst, struct net_buf *nb)
{
	k_fifo_put(&zst->zst_fifo, nb);
	k_work_submit(&zst->zst_work);
}
