/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright Laird Connectivity 2021-2022.
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Dummy transport for the MCUmgr non-SMP over console protocol for unit testing.
 */

/* Define required for uart_mcumgr.h functionality reuse */
#define CONFIG_UART_MCUMGR_RX_BUF_SIZE CONFIG_MCUMGR_TRANSPORT_RAW_DUMMY_RX_BUF_SIZE

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/base64.h>
#include <zephyr/drivers/console/uart_mcumgr.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_raw_dummy.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>
#include <string.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_RAW_DUMMY_RX_BUF_SIZE != 0,
	     "CONFIG_MCUMGR_TRANSPORT_RAW_DUMMY_RX_BUF_SIZE must be > 0");

static struct mcumgr_serial_rx_ctxt smp_dummy_rx_ctxt;
static struct mcumgr_serial_rx_ctxt smp_dummy_tx_ctxt;
static struct smp_transport smp_dummy_transport;
static bool enable_dummy_smp;
static struct k_sem smp_data_ready_sem;
static uint8_t smp_send_buffer[CONFIG_MCUMGR_TRANSPORT_RAW_DUMMY_RX_BUF_SIZE];
static uint16_t smp_send_pos;
static uint8_t smp_receive_buffer[CONFIG_MCUMGR_TRANSPORT_RAW_DUMMY_RX_BUF_SIZE];
static uint16_t smp_receive_pos;

/** Callback to execute when a valid fragment has been received. */
static uart_mcumgr_recv_fn *dummy_mgumgr_recv_cb;

/** Contains the fragment currently being received. */
static struct uart_mcumgr_rx_buf *dummy_mcumgr_cur_buf;

static void smp_raw_dummy_mcumgr_free_rx_buf(struct uart_mcumgr_rx_buf *rx_buf);

static struct net_buf *mcumgr_dummy_process_frag(
	struct mcumgr_serial_rx_ctxt *rx_ctxt,
	const uint8_t *frag, int frag_len);

K_MEM_SLAB_DEFINE(raw_dummy_mcumgr_slab, sizeof(struct uart_mcumgr_rx_buf), 1, 1);

void smp_raw_dummy_clear_state(void)
{
	k_sem_reset(&smp_data_ready_sem);

	memset(smp_receive_buffer, 0, sizeof(smp_receive_buffer));
	smp_receive_pos = 0;
	memset(smp_send_buffer, 0, sizeof(smp_send_buffer));
	smp_send_pos = 0;
}

/**
 * Processes a single line (fragment) coming from the mcumgr UART driver.
 */
static void smp_raw_dummy_process_frag(struct uart_mcumgr_rx_buf *rx_buf)
{
	struct net_buf *nb;

	/* Decode the fragment and write the result to the global receive
	 * context.
	 */
	nb = mcumgr_dummy_process_frag(&smp_dummy_rx_ctxt,
					rx_buf->data, rx_buf->length);

	/* Release the encoded fragment. */
	smp_raw_dummy_mcumgr_free_rx_buf(rx_buf);

	/* If a complete packet has been received, pass it to SMP for
	 * processing.
	 */
	if (nb != NULL) {
		smp_rx_req(&smp_dummy_transport, nb);
	}
}

/**
 * Processes a single line (fragment) coming from the mcumgr response to be
 * used in tests
 */
struct net_buf *smp_raw_dummy_get_outgoing(void)
{

	struct net_buf *nb;

	/* Decode the fragment and write the result to the global receive
	 * context.
	 */
	nb = mcumgr_dummy_process_frag(&smp_dummy_tx_ctxt, smp_send_buffer, smp_send_pos);

	return nb;
}

static uint16_t smp_raw_dummy_get_mtu(const struct net_buf *nb)
{
	return CONFIG_MCUMGR_TRANSPORT_RAW_DUMMY_RX_BUF_SIZE;
}

static int smp_raw_dummy_tx_pkt_int(struct net_buf *nb)
{
	uint16_t data_size = MIN(nb->len, (sizeof(smp_send_buffer) - smp_send_pos - 1));

	if (enable_dummy_smp == true) {
		memcpy(&smp_send_buffer[smp_send_pos], nb->data, data_size);
		smp_send_pos += data_size;

		if (data_size == nb->len) {
			k_sem_give(&smp_data_ready_sem);
		}
	}

	smp_packet_free(nb);

	return 0;
}

static int smp_raw_dummy_init(void)
{
	int rc;

	k_sem_init(&smp_data_ready_sem, 0, 1);

	smp_dummy_transport.functions.output = smp_raw_dummy_tx_pkt_int;
	smp_dummy_transport.functions.get_mtu = smp_raw_dummy_get_mtu;

	rc = smp_transport_init(&smp_dummy_transport);

	if (rc != 0) {
		return rc;
	}

	dummy_mgumgr_recv_cb = smp_raw_dummy_process_frag;

	return 0;
}

static struct uart_mcumgr_rx_buf *smp_raw_dummy_mcumgr_alloc_rx_buf(void)
{
	struct uart_mcumgr_rx_buf *rx_buf;
	void *block;
	int rc;

	rc = k_mem_slab_alloc(&raw_dummy_mcumgr_slab, &block, K_NO_WAIT);
	if (rc != 0) {
		return NULL;
	}

	rx_buf = block;
	rx_buf->length = 0;
	return rx_buf;
}

static void smp_raw_dummy_mcumgr_free_rx_buf(struct uart_mcumgr_rx_buf *rx_buf)
{
	void *block;

	block = rx_buf;
	k_mem_slab_free(&raw_dummy_mcumgr_slab, block);
}

/**
 * Processes a single incoming byte.
 */
static struct uart_mcumgr_rx_buf *smp_raw_dummy_mcumgr_rx_byte(uint8_t byte)
{
	struct uart_mcumgr_rx_buf *rx_buf;

	if (dummy_mcumgr_cur_buf == NULL) {
		dummy_mcumgr_cur_buf = smp_raw_dummy_mcumgr_alloc_rx_buf();
	}

	rx_buf = dummy_mcumgr_cur_buf;

	if (rx_buf->length >= sizeof(rx_buf->data)) {
		smp_raw_dummy_mcumgr_free_rx_buf(dummy_mcumgr_cur_buf);
		dummy_mcumgr_cur_buf = NULL;
	} else {
		rx_buf->data[rx_buf->length++] = byte;
	}

	dummy_mcumgr_cur_buf = NULL;
	return rx_buf;
}

void smp_raw_dummy_mcumgr_add_data(uint8_t *data, uint16_t data_size)
{
	struct uart_mcumgr_rx_buf *rx_buf;
	int i;

	for (i = 0; i < data_size; i++) {
		rx_buf = smp_raw_dummy_mcumgr_rx_byte(data[i]);
		if (rx_buf != NULL) {
			dummy_mgumgr_recv_cb(rx_buf);
		}
	}
}

static void mcumgr_dummy_free_rx_ctxt(struct mcumgr_serial_rx_ctxt *rx_ctxt)
{
	if (rx_ctxt->nb != NULL) {
		smp_packet_free(rx_ctxt->nb);
		rx_ctxt->nb = NULL;
	}
}

/**
 * Processes a received mcumgr fragment
 *
 * @param rx_ctxt    The receive context
 * @param frag       The fragment buffer
 * @param frag_len   The size of the fragment
 *
 * @return   The net buffer if a complete packet was received, NULL if
 *   the frame is invalid or if additional fragments are
 *   expected
 */
static struct net_buf *mcumgr_dummy_process_frag(
	struct mcumgr_serial_rx_ctxt *rx_ctxt,
	const uint8_t *frag, int frag_len)
{
	struct net_buf *nb;
	struct smp_hdr *rec_hdr;
	uint16_t total_size;

	if (rx_ctxt->nb == NULL) {
		rx_ctxt->nb = smp_packet_alloc();
		net_buf_reset(rx_ctxt->nb);
		if (rx_ctxt->nb == NULL) {
			return NULL;
		}
	}


	net_buf_add_mem(rx_ctxt->nb, frag, frag_len);

	if (rx_ctxt->nb->len < sizeof(struct smp_hdr)) {
		/* Missing header */
		return NULL;
	}

	/*
	 * Perform some basic cursory checks to ensure some fields of the packet are
	 * actually valid.
	 */
	rec_hdr = (struct smp_hdr *)rx_ctxt->nb->data;
	total_size = sys_be16_to_cpu(rec_hdr->nh_len) + sizeof(struct smp_hdr);

	if (total_size > CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE) {
		/* Payload is longer than the maximum supported MTU. */
		mcumgr_dummy_free_rx_ctxt(rx_ctxt);
		return NULL;
	} else if (rec_hdr->nh_op >= MGMT_OP_COUNT) {
		/* Unknown op-code, likely not a valid MCUmgr message. */
		mcumgr_dummy_free_rx_ctxt(rx_ctxt);
		return NULL;
	} else if (rx_ctxt->nb->len < total_size) {
		/* More fragments expected. */
		return NULL;
	} else if (rx_ctxt->nb->len > total_size) {
		/* Payload longer than indicated in header. */
		mcumgr_dummy_free_rx_ctxt(rx_ctxt);
		return NULL;
	}

	nb = rx_ctxt->nb;
	rx_ctxt->nb = NULL;
	return nb;
}

bool smp_raw_dummy_wait_for_data(uint32_t wait_time_s)
{
	return k_sem_take(&smp_data_ready_sem, K_SECONDS(wait_time_s)) == 0 ? true : false;
}

void smp_raw_dummy_add_data(void)
{
	smp_raw_dummy_mcumgr_add_data(smp_receive_buffer, smp_receive_pos);
}

uint16_t smp_raw_dummy_get_send_pos(void)
{
	return smp_send_pos;
}

uint16_t smp_raw_dummy_get_receive_pos(void)
{
	return smp_receive_pos;
}

int smp_raw_dummy_tx_pkt(const uint8_t *data, int len)
{
	uint16_t data_size =
		MIN(len, (sizeof(smp_receive_buffer) - smp_receive_pos - 1));

	if (enable_dummy_smp == true) {
		memcpy(&smp_receive_buffer[smp_receive_pos], data, data_size);
		smp_receive_pos += data_size;
	}

	return 0;
}

void smp_raw_dummy_enable(void)
{
	enable_dummy_smp = true;
}

void smp_raw_dummy_disable(void)
{
	enable_dummy_smp = false;
}

bool smp_raw_dummy_get_status(void)
{
	return enable_dummy_smp;
}

SYS_INIT(smp_raw_dummy_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
