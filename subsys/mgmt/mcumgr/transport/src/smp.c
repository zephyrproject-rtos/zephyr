/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>

#include <mgmt/mcumgr/transport/smp_reassembly.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcumgr_smp, CONFIG_MCUMGR_TRANSPORT_LOG_LEVEL);

/* To be able to unit test some callers some functions need to be
 * demoted to allow overriding them.
 */
#ifdef CONFIG_ZTEST
#define WEAK __weak
#else
#define WEAK
#endif

K_THREAD_STACK_DEFINE(smp_work_queue_stack, CONFIG_MCUMGR_TRANSPORT_WORKQUEUE_STACK_SIZE);

static struct k_work_q smp_work_queue;

#ifdef CONFIG_SMP_CLIENT
static sys_slist_t smp_transport_clients;
#endif

static const struct k_work_queue_config smp_work_queue_config = {
	.name = "mcumgr smp"
};

NET_BUF_POOL_DEFINE(pkt_pool, CONFIG_MCUMGR_TRANSPORT_NETBUF_COUNT,
		    CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE,
		    CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE, NULL);

struct net_buf *smp_packet_alloc(void)
{
	return net_buf_alloc(&pkt_pool, K_NO_WAIT);
}

void smp_packet_free(struct net_buf *nb)
{
	net_buf_unref(nb);
}

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
void *smp_alloc_rsp(const void *req, void *arg)
{
	const struct net_buf *req_nb;
	struct net_buf *rsp_nb;
	struct smp_transport *smpt = arg;

	req_nb = req;

	rsp_nb = smp_packet_alloc();
	if (rsp_nb == NULL) {
		return NULL;
	}

	if (smpt->functions.ud_copy) {
		smpt->functions.ud_copy(rsp_nb, req_nb);
	} else {
		memcpy(net_buf_user_data(rsp_nb),
		       net_buf_user_data((void *)req_nb),
		       req_nb->user_data_size);
	}

	return rsp_nb;
}

void smp_free_buf(void *buf, void *arg)
{
	struct smp_transport *smpt = arg;

	if (!buf) {
		return;
	}

	if (smpt->functions.ud_free) {
		smpt->functions.ud_free(net_buf_user_data((struct net_buf *)buf));
	}

	smp_packet_free(buf);
}

/**
 * Processes a single SMP packet and sends the corresponding response(s).
 */
static int
smp_process_packet(struct smp_transport *smpt, struct net_buf *nb)
{
	struct cbor_nb_reader reader;
	struct cbor_nb_writer writer;
	struct smp_streamer streamer;
	int rc;

	streamer = (struct smp_streamer) {
		.reader = &reader,
		.writer = &writer,
		.smpt = smpt,
	};

	rc = smp_process_request_packet(&streamer, nb);
	return rc;
}

/**
 * Processes all received SNP request packets.
 */
static void
smp_handle_reqs(struct k_work *work)
{
	struct smp_transport *smpt;
	struct net_buf *nb;

	smpt = (void *)work;

	/* Read and handle received messages */
	while ((nb = net_buf_get(&smpt->fifo, K_NO_WAIT)) != NULL) {
		smp_process_packet(smpt, nb);
	}
}

int smp_transport_init(struct smp_transport *smpt)
{
	__ASSERT((smpt->functions.output != NULL),
		 "Required transport output function pointer cannot be NULL");

	if (smpt->functions.output == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_MCUMGR_TRANSPORT_REASSEMBLY
	smp_reassembly_init(smpt);
#endif

	k_work_init(&smpt->work, smp_handle_reqs);
	k_fifo_init(&smpt->fifo);

	return 0;
}

#ifdef CONFIG_SMP_CLIENT
struct smp_transport *smp_client_transport_get(int smpt_type)
{
	struct smp_client_transport_entry *entry;

	SYS_SLIST_FOR_EACH_CONTAINER(&smp_transport_clients, entry, node) {
		if (entry->smpt_type == smpt_type) {
			return entry->smpt;
		}
	}

	return NULL;
}

void smp_client_transport_register(struct smp_client_transport_entry *entry)
{
	if (smp_client_transport_get(entry->smpt_type)) {
		/* Already in list */
		return;
	}

	sys_slist_append(&smp_transport_clients, &entry->node);

}

#endif /* CONFIG_SMP_CLIENT */

/**
 * @brief Enqueues an incoming SMP request packet for processing.
 *
 * This function always consumes the supplied net_buf.
 *
 * @param smpt                  The transport to use to send the corresponding
 *                                  response(s).
 * @param nb                    The request packet to process.
 */
WEAK void
smp_rx_req(struct smp_transport *smpt, struct net_buf *nb)
{
	net_buf_put(&smpt->fifo, nb);
	k_work_submit_to_queue(&smp_work_queue, &smpt->work);
}

#ifdef CONFIG_SMP_CLIENT
void smp_tx_req(struct k_work *work)
{
	k_work_submit_to_queue(&smp_work_queue, work);
}
#endif

void smp_rx_remove_invalid(struct smp_transport *zst, void *arg)
{
	struct net_buf *nb;
	struct k_fifo temp_fifo;

	if (zst->functions.query_valid_check == NULL) {
		/* No check check function registered, abort check */
		return;
	}

	/* Cancel current work-queue if ongoing */
	if (k_work_busy_get(&zst->work) & (K_WORK_RUNNING | K_WORK_QUEUED)) {
		k_work_cancel(&zst->work);
	}

	/* Run callback function and remove all buffers that are no longer needed. Store those
	 * that are in a temporary FIFO
	 */
	k_fifo_init(&temp_fifo);

	while ((nb = net_buf_get(&zst->fifo, K_NO_WAIT)) != NULL) {
		if (!zst->functions.query_valid_check(nb, arg)) {
			smp_free_buf(nb, zst);
		} else {
			net_buf_put(&temp_fifo, nb);
		}
	}

	/* Re-insert the remaining queued operations into the original FIFO */
	while ((nb = net_buf_get(&temp_fifo, K_NO_WAIT)) != NULL) {
		net_buf_put(&zst->fifo, nb);
	}

	/* If at least one entry remains, queue the workqueue for running */
	if (!k_fifo_is_empty(&zst->fifo)) {
		k_work_submit_to_queue(&smp_work_queue, &zst->work);
	}
}

void smp_rx_clear(struct smp_transport *zst)
{
	struct net_buf *nb;

	/* Cancel current work-queue if ongoing */
	if (k_work_busy_get(&zst->work) & (K_WORK_RUNNING | K_WORK_QUEUED)) {
		k_work_cancel(&zst->work);
	}

	/* Drain the FIFO of all entries without re-adding any */
	while ((nb = net_buf_get(&zst->fifo, K_NO_WAIT)) != NULL) {
		smp_free_buf(nb, zst);
	}
}

static int smp_init(void)
{
#ifdef CONFIG_SMP_CLIENT
	sys_slist_init(&smp_transport_clients);
#endif

	k_work_queue_init(&smp_work_queue);

	k_work_queue_start(&smp_work_queue, smp_work_queue_stack,
			   K_THREAD_STACK_SIZEOF(smp_work_queue_stack),
			   CONFIG_MCUMGR_TRANSPORT_WORKQUEUE_THREAD_PRIO, &smp_work_queue_config);

	return 0;
}

SYS_INIT(smp_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
