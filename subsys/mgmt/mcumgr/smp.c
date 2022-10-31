/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
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

K_THREAD_STACK_DEFINE(smp_work_queue_stack, CONFIG_MCUMGR_SMP_WORKQUEUE_STACK_SIZE);

static struct k_work_q smp_work_queue;

static const struct k_work_queue_config smp_work_queue_config = {
	.name = "mcumgr smp"
};

/**
 * @brief Allocates a response buffer.
 *
 * If a source buf is provided, its user data is copied into the new buffer.
 *
 * @param req		An optional source buffer to copy user data from.
 * @param arg		The streamer providing the callback.
 *
 * @return	Newly-allocated buffer on success
 *		NULL on failure.
 */
void *zephyr_smp_alloc_rsp(const void *req, void *arg)
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

void zephyr_smp_free_buf(void *buf, void *arg)
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
	struct net_buf *nb;

	zst = arg;
	nb = rsp;

	/* Pass full packet to output function so it can be transmitted or split into frames as
	 * needed by the transport
	 */
	return zst->zst_output(nb);
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

void
zephyr_smp_transport_init(struct zephyr_smp_transport *zst,
			  zephyr_smp_transport_out_fn *output_func,
			  zephyr_smp_transport_get_mtu_fn *get_mtu_func,
			  zephyr_smp_transport_ud_copy_fn *ud_copy_func,
			  zephyr_smp_transport_ud_free_fn *ud_free_func,
			  zephyr_smp_transport_query_valid_check_fn *query_valid_check_func)
{
	*zst = (struct zephyr_smp_transport) {
		.zst_output = output_func,
		.zst_get_mtu = get_mtu_func,
		.zst_ud_copy = ud_copy_func,
		.zst_ud_free = ud_free_func,
		.zst_query_valid_check = query_valid_check_func,
	};

#ifdef CONFIG_MCUMGR_SMP_REASSEMBLY
	zephyr_smp_reassembly_init(zst);
#endif

	k_work_init(&zst->zst_work, zephyr_smp_handle_reqs);
	k_fifo_init(&zst->zst_fifo);
}

void smp_rx_remove_invalid(struct zephyr_smp_transport *zst, void *arg)
{
	struct net_buf *nb;
	struct k_fifo temp_fifo;

	if (zst->zst_query_valid_check == NULL) {
		/* No check check function registered, abort check */
		return;
	}

	/* Cancel current work-queue if ongoing */
	if (k_work_busy_get(&zst->zst_work) & (K_WORK_RUNNING | K_WORK_QUEUED)) {
		k_work_cancel(&zst->zst_work);
	}

	/* Run callback function and remove all buffers that are no longer needed. Store those
	 * that are in a temporary FIFO
	 */
	k_fifo_init(&temp_fifo);

	while ((nb = net_buf_get(&zst->zst_fifo, K_NO_WAIT)) != NULL) {
		if (zst->zst_query_valid_check(nb, arg)) {
			zephyr_smp_free_buf(nb, zst);
		} else {
			net_buf_put(&temp_fifo, nb);
		}
	}

	/* Re-insert the remaining queued operations into the original FIFO */
	while ((nb = net_buf_get(&temp_fifo, K_NO_WAIT)) != NULL) {
		net_buf_put(&zst->zst_fifo, nb);
	}

	/* If at least one entry remains, queue the workqueue for running */
	if (!k_fifo_is_empty(&zst->zst_fifo)) {
		k_work_submit_to_queue(&smp_work_queue, &zst->zst_work);
	}
}

/**
 * @brief Enqueues an incoming SMP request packet for processing.
 *
 * This function always consumes the supplied net_buf.
 *
 * @param zst                   The transport to use to send the corresponding
 *                                  response(s).
 * @param nb                    The request packet to process.
 */
WEAK void
zephyr_smp_rx_req(struct zephyr_smp_transport *zst, struct net_buf *nb)
{
	net_buf_put(&zst->zst_fifo, nb);
	k_work_submit_to_queue(&smp_work_queue, &zst->zst_work);
}

static int zephyr_smp_init(const struct device *dev)
{
	k_work_queue_init(&smp_work_queue);

	k_work_queue_start(&smp_work_queue, smp_work_queue_stack,
			   K_THREAD_STACK_SIZEOF(smp_work_queue_stack),
			   CONFIG_MCUMGR_SMP_WORKQUEUE_THREAD_PRIO, &smp_work_queue_config);

	return 0;
}

SYS_INIT(zephyr_smp_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
