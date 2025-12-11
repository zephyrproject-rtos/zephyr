/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net_buf.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/sys/byteorder.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#include "smp_stub.h"

K_THREAD_STACK_DEFINE(smp_stub_work_queue_stack, CONFIG_MCUMGR_TRANSPORT_WORKQUEUE_STACK_SIZE);

static mcmgr_client_data_check_fn rx_verify_cb;
static int send_client_failure;
static struct net_buf *response_buf;
static struct smp_hdr res_hdr;
static struct smp_transport smpt_test;
static struct smp_client_transport_entry smp_client_transport;
static struct k_work_q smp_work_queue;
static struct k_work stub_work;

static const struct k_work_queue_config smp_work_queue_config = {
	.name = "mcumgr smp"
};

void smp_stub_set_rx_data_verify(mcmgr_client_data_check_fn cb)
{
	rx_verify_cb = cb;
}

void smp_client_send_status_stub(int status)
{
	send_client_failure = status;
}

struct net_buf *smp_response_buf_allocation(void)
{
	smp_client_response_buf_clean();

	response_buf = smp_packet_alloc();

	return response_buf;
}

void smp_client_response_buf_clean(void)
{
	if (response_buf) {
		smp_client_buf_free(response_buf);
		response_buf = NULL;
	}
}

void smp_transport_read_hdr(const struct net_buf *nb, struct smp_hdr *dst_hdr)
{
	memcpy(dst_hdr, nb->data, sizeof(*dst_hdr));
	dst_hdr->nh_len = sys_be16_to_cpu(dst_hdr->nh_len);
	dst_hdr->nh_group = sys_be16_to_cpu(dst_hdr->nh_group);
}


static uint16_t smp_uart_get_mtu(const struct net_buf *nb)
{
	return 256;
}

static int smp_uart_tx_pkt(struct net_buf *nb)
{
	if (send_client_failure) {
		/* Test Send cmd fail */
		return send_client_failure;
	}

	memcpy(&res_hdr, nb->data, sizeof(res_hdr));
	res_hdr.nh_len = sys_be16_to_cpu(res_hdr.nh_len);
	res_hdr.nh_group = sys_be16_to_cpu(res_hdr.nh_group);
	res_hdr.nh_op += 1; /* Request to response */

	/* Validate Input data if callback is configured */
	if (rx_verify_cb) {
		rx_verify_cb(nb);
	}

	/* Free tx buf */
	net_buf_unref(nb);

	if (response_buf) {
		k_work_submit_to_queue(&smp_work_queue, &stub_work);
	}

	return 0;
}

static void smp_client_handle_reqs(struct k_work *work)
{
	if (response_buf) {
		smp_client_single_response(response_buf, &res_hdr);
	}
}

void stub_smp_client_transport_register(void)
{

	smpt_test.functions.output = smp_uart_tx_pkt;
	smpt_test.functions.get_mtu = smp_uart_get_mtu;

	smp_transport_init(&smpt_test);
	smp_client_transport.smpt = &smpt_test;
	smp_client_transport.smpt_type = SMP_SERIAL_TRANSPORT;
	smp_client_transport_register(&smp_client_transport);


	k_work_queue_init(&smp_work_queue);

	k_work_queue_start(&smp_work_queue, smp_stub_work_queue_stack,
			   K_THREAD_STACK_SIZEOF(smp_stub_work_queue_stack),
			   CONFIG_MCUMGR_TRANSPORT_WORKQUEUE_THREAD_PRIO, &smp_work_queue_config);

	k_work_init(&stub_work, smp_client_handle_reqs);
}
