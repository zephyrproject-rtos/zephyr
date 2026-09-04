/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net_buf.h>
#include <string.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <mgmt/mcumgr/transport/smp_internal.h>

static struct smp_transport smpt_test;
static struct smp_client_transport_entry smp_client_transport;

/*
 * A second transport, registered nowhere and owned by no client. It stands in for the
 * other transport a device that is both an SMP server and an SMP client has, and exists
 * so that a response can be fed in on a transport that no pending command went out on.
 */
static struct smp_transport smpt_other;

/* Stubbed functions */

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
	smp_packet_free(nb);
	return 0;
}

void stub_smp_client_transport_register(void)
{

	smpt_test.functions.output = smp_uart_tx_pkt;
	smpt_test.functions.get_mtu = smp_uart_get_mtu;

	smp_transport_init(&smpt_test);
	smp_client_transport.smpt = &smpt_test;
	smp_client_transport.smpt_type = SMP_SERIAL_TRANSPORT;
	smp_client_transport_register(&smp_client_transport);

	smpt_other.functions.output = smp_uart_tx_pkt;
	smpt_other.functions.get_mtu = smp_uart_get_mtu;
	smp_transport_init(&smpt_other);
}

struct smp_transport *stub_smp_other_transport_get(void)
{
	return &smpt_other;
}
