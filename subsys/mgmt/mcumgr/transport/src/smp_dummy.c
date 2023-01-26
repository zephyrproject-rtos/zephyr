/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright Laird Connectivity 2021-2022.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Dummy transport for the mcumgr SMP protocol for unit testing.
 */

/* Define required for uart_mcumgr.h functionality reuse */
#define CONFIG_UART_MCUMGR_RX_BUF_SIZE CONFIG_MCUMGR_TRANSPORT_DUMMY_RX_BUF_SIZE
#define MCUMGR_DUMMY_MAX_FRAME CONFIG_MCUMGR_TRANSPORT_DUMMY_RX_BUF_SIZE

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/base64.h>
#include <zephyr/drivers/console/uart_mcumgr.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>
#include <string.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_DUMMY_RX_BUF_SIZE != 0,
	     "CONFIG_MCUMGR_TRANSPORT_DUMMY_RX_BUF_SIZE must be > 0");

struct device;
static struct mcumgr_serial_rx_ctxt smp_dummy_rx_ctxt;
static struct mcumgr_serial_rx_ctxt smp_dummy_tx_ctxt;
static struct smp_transport smp_dummy_transport;
static bool enable_dummy_smp;
static struct k_sem smp_data_ready_sem;
static uint8_t smp_send_buffer[CONFIG_MCUMGR_TRANSPORT_DUMMY_RX_BUF_SIZE];
static uint16_t smp_send_pos;
static uint8_t smp_receive_buffer[CONFIG_MCUMGR_TRANSPORT_DUMMY_RX_BUF_SIZE];
static uint16_t smp_receive_pos;

/** Callback to execute when a valid fragment has been received. */
static uart_mcumgr_recv_fn *dummy_mgumgr_recv_cb;

/** Contains the fragment currently being received. */
static struct uart_mcumgr_rx_buf *dummy_mcumgr_cur_buf;

/**
 * Whether the line currently being read should be ignored.  This is true if
 * the line is too long or if there is no buffer available to hold it.
 */
static bool dummy_mcumgr_ignoring;

static void smp_dummy_process_rx_queue(struct k_work *work);
static void dummy_mcumgr_free_rx_buf(struct uart_mcumgr_rx_buf *rx_buf);


static struct net_buf *mcumgr_dummy_process_frag(
	struct mcumgr_serial_rx_ctxt *rx_ctxt,
	const uint8_t *frag, int frag_len);

static struct net_buf *mcumgr_dummy_process_frag_outgoing(
	struct mcumgr_serial_rx_ctxt *tx_ctxt,
	const uint8_t *frag, int frag_len);

static int mcumgr_dummy_tx_pkt(const uint8_t *data, int len,
			       mcumgr_serial_tx_cb cb);

K_FIFO_DEFINE(smp_dummy_rx_fifo);
K_WORK_DEFINE(smp_dummy_work, smp_dummy_process_rx_queue);
K_MEM_SLAB_DEFINE(dummy_mcumgr_slab, sizeof(struct uart_mcumgr_rx_buf), 1, 1);

void smp_dummy_clear_state(void)
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
static void smp_dummy_process_frag(struct uart_mcumgr_rx_buf *rx_buf)
{
	struct net_buf *nb;

	/* Decode the fragment and write the result to the global receive
	 * context.
	 */
	nb = mcumgr_dummy_process_frag(&smp_dummy_rx_ctxt,
					rx_buf->data, rx_buf->length);

	/* Release the encoded fragment. */
	dummy_mcumgr_free_rx_buf(rx_buf);

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
static struct net_buf *smp_dummy_process_frag_outgoing(uint8_t *buffer,
						       uint8_t buffer_size)
{
	struct net_buf *nb;

	/* Decode the fragment and write the result to the global receive
	 * context.
	 */
	nb = mcumgr_dummy_process_frag_outgoing(&smp_dummy_tx_ctxt,
					buffer, buffer_size);

	return nb;
}

static void smp_dummy_process_rx_queue(struct k_work *work)
{
	struct uart_mcumgr_rx_buf *rx_buf;

	while ((rx_buf = k_fifo_get(&smp_dummy_rx_fifo, K_NO_WAIT)) != NULL) {
		smp_dummy_process_frag(rx_buf);
	}
}

struct net_buf *smp_dummy_get_outgoing(void)
{
	return smp_dummy_process_frag_outgoing(smp_send_buffer, smp_send_pos);
}

/**
 * Enqueues a received SMP fragment for later processing.  This function
 * executes in the interrupt context.
 */
static void smp_dummy_rx_frag(struct uart_mcumgr_rx_buf *rx_buf)
{
	k_fifo_put(&smp_dummy_rx_fifo, rx_buf);
	k_work_submit(&smp_dummy_work);
}

static uint16_t smp_dummy_get_mtu(const struct net_buf *nb)
{
	return CONFIG_MCUMGR_TRANSPORT_DUMMY_RX_BUF_SIZE;
}

int dummy_mcumgr_send_raw(const void *data, int len)
{
	uint16_t data_size =
	MIN(len, (sizeof(smp_send_buffer) - smp_send_pos - 1));

	if (enable_dummy_smp == true) {
		memcpy(&smp_send_buffer[smp_send_pos], data, data_size);
		smp_send_pos += data_size;

		if (smp_send_buffer[(smp_send_pos - 1)] == 0x0a) {
			/* End character of SMP over console message has been
			 * received
			 */
			k_sem_give(&smp_data_ready_sem);
		}
	}

	return 0;
}

static int smp_dummy_tx_pkt_int(struct net_buf *nb)
{
	int rc;

	rc = mcumgr_dummy_tx_pkt(nb->data, nb->len, dummy_mcumgr_send_raw);
	smp_packet_free(nb);

	return rc;
}

static int smp_dummy_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_sem_init(&smp_data_ready_sem, 0, 1);

	smp_transport_init(&smp_dummy_transport, smp_dummy_tx_pkt_int,
			   smp_dummy_get_mtu, NULL, NULL, NULL);
	dummy_mgumgr_recv_cb = smp_dummy_rx_frag;

	return 0;
}


static struct uart_mcumgr_rx_buf *dummy_mcumgr_alloc_rx_buf(void)
{
	struct uart_mcumgr_rx_buf *rx_buf;
	void *block;
	int rc;

	rc = k_mem_slab_alloc(&dummy_mcumgr_slab, &block, K_NO_WAIT);
	if (rc != 0) {
		return NULL;
	}

	rx_buf = block;
	rx_buf->length = 0;
	return rx_buf;
}

static void dummy_mcumgr_free_rx_buf(struct uart_mcumgr_rx_buf *rx_buf)
{
	void *block;

	block = rx_buf;
	k_mem_slab_free(&dummy_mcumgr_slab, &block);
}

/**
 * Processes a single incoming byte.
 */
static struct uart_mcumgr_rx_buf *dummy_mcumgr_rx_byte(uint8_t byte)
{
	struct uart_mcumgr_rx_buf *rx_buf;

	if (!dummy_mcumgr_ignoring) {
		if (dummy_mcumgr_cur_buf == NULL) {
			dummy_mcumgr_cur_buf = dummy_mcumgr_alloc_rx_buf();
			if (dummy_mcumgr_cur_buf == NULL) {
				/* Insufficient buffers; drop this fragment. */
				dummy_mcumgr_ignoring = true;
			}
		}
	}

	rx_buf = dummy_mcumgr_cur_buf;
	if (!dummy_mcumgr_ignoring) {
		if (rx_buf->length >= sizeof(rx_buf->data)) {
			/* Line too long; drop this fragment. */
			dummy_mcumgr_free_rx_buf(dummy_mcumgr_cur_buf);
			dummy_mcumgr_cur_buf = NULL;
			dummy_mcumgr_ignoring = true;
		} else {
			rx_buf->data[rx_buf->length++] = byte;
		}
	}

	if (byte == '\n') {
		/* Fragment complete. */
		if (dummy_mcumgr_ignoring) {
			dummy_mcumgr_ignoring = false;
		} else {
			dummy_mcumgr_cur_buf = NULL;
			return rx_buf;
		}
	}

	return NULL;
}

void dummy_mcumgr_add_data(uint8_t *data, uint16_t data_size)
{
	struct uart_mcumgr_rx_buf *rx_buf;
	int i;

	for (i = 0; i < data_size; i++) {
		rx_buf = dummy_mcumgr_rx_byte(data[i]);
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

static uint16_t mcumgr_dummy_calc_crc(const uint8_t *data, int len)
{
	return crc16_itu_t(0x0000, data, len);
}

static int mcumgr_dummy_parse_op(const uint8_t *buf, int len)
{
	uint16_t op;

	if (len < sizeof(op)) {
		return -EINVAL;
	}

	memcpy(&op, buf, sizeof(op));
	op = sys_be16_to_cpu(op);

	if (op != MCUMGR_SERIAL_HDR_PKT && op != MCUMGR_SERIAL_HDR_FRAG) {
		return -EINVAL;
	}

	return op;
}

static int mcumgr_dummy_extract_len(struct mcumgr_serial_rx_ctxt *rx_ctxt)
{
	if (rx_ctxt->nb->len < 2) {
		return -EINVAL;
	}

	rx_ctxt->pkt_len = net_buf_pull_be16(rx_ctxt->nb);
	return 0;
}

static int mcumgr_dummy_decode_frag(struct mcumgr_serial_rx_ctxt *rx_ctxt,
				     const uint8_t *frag, int frag_len)
{
	size_t dec_len;
	int rc;

	rc = base64_decode(rx_ctxt->nb->data + rx_ctxt->nb->len,
				   net_buf_tailroom(rx_ctxt->nb), &dec_len,
				   frag, frag_len);
	if (rc != 0) {
		return -EINVAL;
	}

	rx_ctxt->nb->len += dec_len;

	return 0;
}

/**
 * Processes a received mcumgr fragment
 *
 * @param rx_ctxt    The receive context
 * @param frag       The fragment buffer
 * @param frag_len   The size of the fragment
 *
 * @return           The net buffer if a complete packet was received, NULL if
 *                   the frame is invalid or if additional fragments are
 *                   expected
 */
static struct net_buf *mcumgr_dummy_process_frag(
	struct mcumgr_serial_rx_ctxt *rx_ctxt,
	const uint8_t *frag, int frag_len)
{
	struct net_buf *nb;
	uint16_t crc;
	uint16_t op;
	int rc;

	if (rx_ctxt->nb == NULL) {
		rx_ctxt->nb = smp_packet_alloc();
		if (rx_ctxt->nb == NULL) {
			return NULL;
		}
	}

	op = mcumgr_dummy_parse_op(frag, frag_len);
	switch (op) {
	case MCUMGR_SERIAL_HDR_PKT:
		net_buf_reset(rx_ctxt->nb);
		break;

	case MCUMGR_SERIAL_HDR_FRAG:
		if (rx_ctxt->nb->len == 0U) {
			mcumgr_dummy_free_rx_ctxt(rx_ctxt);
			return NULL;
		}
		break;

	default:
		return NULL;
	}

	rc = mcumgr_dummy_decode_frag(rx_ctxt,
				       frag + sizeof(op),
				       frag_len - sizeof(op));
	if (rc != 0) {
		mcumgr_dummy_free_rx_ctxt(rx_ctxt);
		return NULL;
	}

	if (op == MCUMGR_SERIAL_HDR_PKT) {
		rc = mcumgr_dummy_extract_len(rx_ctxt);
		if (rc < 0) {
			mcumgr_dummy_free_rx_ctxt(rx_ctxt);
			return NULL;
		}
	}

	if (rx_ctxt->nb->len < rx_ctxt->pkt_len) {
		/* More fragments expected. */
		return NULL;
	}

	if (rx_ctxt->nb->len > rx_ctxt->pkt_len) {
		/* Payload longer than indicated in header. */
		mcumgr_dummy_free_rx_ctxt(rx_ctxt);
		return NULL;
	}

	crc = mcumgr_dummy_calc_crc(rx_ctxt->nb->data, rx_ctxt->nb->len);
	if (crc != 0U) {
		mcumgr_dummy_free_rx_ctxt(rx_ctxt);
		return NULL;
	}

	/* Packet is complete; strip the CRC. */
	rx_ctxt->nb->len -= 2U;

	nb = rx_ctxt->nb;
	rx_ctxt->nb = NULL;
	return nb;
}

/**
 * Processes a mcumgr response fragment
 *
 * @param tx_ctxt    The transmission context
 * @param frag       The fragment buffer
 * @param frag_len   The size of the fragment
 *
 * @return           The net buffer if a complete packet was received, NULL if
 *                   the frame is invalid or if additional fragments are
 *                   expected
 */
static struct net_buf *mcumgr_dummy_process_frag_outgoing(
	struct mcumgr_serial_rx_ctxt *tx_ctxt,
	const uint8_t *frag, int frag_len)
{
	struct net_buf *nb;
	uint16_t crc;
	uint16_t op;
	int rc;

	if (tx_ctxt->nb == NULL) {
		tx_ctxt->nb = smp_packet_alloc();
		if (tx_ctxt->nb == NULL) {
			return NULL;
		}
	}

	op = mcumgr_dummy_parse_op(frag, frag_len);
	switch (op) {
	case MCUMGR_SERIAL_HDR_PKT:
		net_buf_reset(tx_ctxt->nb);
		break;

	case MCUMGR_SERIAL_HDR_FRAG:
		if (tx_ctxt->nb->len == 0U) {
			mcumgr_dummy_free_rx_ctxt(tx_ctxt);
			return NULL;
		}
		break;

	default:
		return NULL;
	}

	rc = mcumgr_dummy_decode_frag(tx_ctxt,
				       frag + sizeof(op),
				       frag_len - sizeof(op));
	if (rc != 0) {
		mcumgr_dummy_free_rx_ctxt(tx_ctxt);
		return NULL;
	}

	if (op == MCUMGR_SERIAL_HDR_PKT) {
		rc = mcumgr_dummy_extract_len(tx_ctxt);
		if (rc < 0) {
			mcumgr_dummy_free_rx_ctxt(tx_ctxt);
			return NULL;
		}
	}

	if (tx_ctxt->nb->len < tx_ctxt->pkt_len) {
		/* More fragments expected. */
		return NULL;
	}

	if (tx_ctxt->nb->len > tx_ctxt->pkt_len) {
		/* Payload longer than indicated in header. */
		mcumgr_dummy_free_rx_ctxt(tx_ctxt);
		return NULL;
	}

	crc = mcumgr_dummy_calc_crc(tx_ctxt->nb->data, tx_ctxt->nb->len);
	if (crc != 0U) {
		mcumgr_dummy_free_rx_ctxt(tx_ctxt);
		return NULL;
	}

	/* Packet is complete; strip the CRC. */
	tx_ctxt->nb->len -= 2U;

	nb = tx_ctxt->nb;
	tx_ctxt->nb = NULL;
	return nb;
}

/**
 * Base64-encodes a small chunk of data and transmits it.  The data must be no
 * larger than three bytes.
 */
static int mcumgr_dummy_tx_small(const void *data, int len,
				  mcumgr_serial_tx_cb cb)
{
	uint8_t b64[4 + 1]; /* +1 required for null terminator. */
	size_t dst_len;
	int rc;

	rc = base64_encode(b64, sizeof(b64), &dst_len, data, len);
	__ASSERT_NO_MSG(rc == 0);
	__ASSERT_NO_MSG(dst_len == 4);

	return cb(b64, 4);
}

/**
 * @brief Transmits a single mcumgr frame over serial.
 *
 * @param data                  The frame payload to transmit.  This does not
 *                                  include a header or CRC.
 * @param first                 Whether this is the first frame in the packet.
 * @param len                   The number of untransmitted data bytes in the
 *                                  packet.
 * @param crc                   The 16-bit CRC of the entire packet.
 * @param cb                    A callback used for transmitting raw data.
 * @param out_data_bytes_txed   On success, the number of data bytes
 *                                  transmitted gets written here.
 *
 * @return                      0 on success; negative error code on failure.
 */
int mcumgr_dummy_tx_frame(const uint8_t *data, bool first, int len,
			   uint16_t crc, mcumgr_serial_tx_cb cb,
			   int *out_data_bytes_txed)
{
	uint8_t raw[3];
	uint16_t u16;
	int dst_off;
	int src_off;
	int rem;
	int rc;

	src_off = 0;
	dst_off = 0;

	if (first) {
		u16 = sys_cpu_to_be16(MCUMGR_SERIAL_HDR_PKT);
	} else {
		u16 = sys_cpu_to_be16(MCUMGR_SERIAL_HDR_FRAG);
	}

	rc = cb(&u16, sizeof(u16));
	if (rc != 0) {
		return rc;
	}
	dst_off += 2;

	/* Only the first fragment contains the packet length. */
	if (first) {
		u16 = sys_cpu_to_be16(len + 2); /* Bug fix to account for CRC */
		memcpy(raw, &u16, sizeof(u16));
		raw[2] = data[0];

		rc = mcumgr_dummy_tx_small(raw, 3, cb);
		if (rc != 0) {
			return rc;
		}

		src_off++;
		dst_off += 4;
	}

	while (1) {
		if (dst_off >= MCUMGR_DUMMY_MAX_FRAME - 4) {
			/* Can't fit any more data in this frame. */
			break;
		}

		/* If we have reached the end of the packet, we need to encode
		 * and send the CRC.
		 */
		rem = len - src_off;
		if (rem == 0) {
			raw[0] = (crc & 0xff00) >> 8;
			raw[1] = crc & 0x00ff;
			rc = mcumgr_dummy_tx_small(raw, 2, cb);
			if (rc != 0) {
				return rc;
			}
			break;
		}

		if (rem == 1) {
			raw[0] = data[src_off];
			src_off++;

			raw[1] = (crc & 0xff00) >> 8;
			raw[2] = crc & 0x00ff;
			rc = mcumgr_dummy_tx_small(raw, 3, cb);
			if (rc != 0) {
				return rc;
			}
			break;
		}

		if (rem == 2) {
			raw[0] = data[src_off];
			raw[1] = data[src_off + 1];
			src_off += 2;

			raw[2] = (crc & 0xff00) >> 8;
			rc = mcumgr_dummy_tx_small(raw, 3, cb);
			if (rc != 0) {
				return rc;
			}

			raw[0] = crc & 0x00ff;
			rc = mcumgr_dummy_tx_small(raw, 1, cb);
			if (rc != 0) {
				return rc;
			}
			break;
		}

		/* Otherwise, just encode payload data. */
		memcpy(raw, data + src_off, 3);
		rc = mcumgr_dummy_tx_small(raw, 3, cb);
		if (rc != 0) {
			return rc;
		}
		src_off += 3;
		dst_off += 4;
	}

	rc = cb("\n", 1);
	if (rc != 0) {
		return rc;
	}

	*out_data_bytes_txed = src_off;
	return 0;
}

static int mcumgr_dummy_tx_pkt(const uint8_t *data, int len, mcumgr_serial_tx_cb cb)
{
	uint16_t crc;
	int data_bytes_txed;
	int src_off;
	int rc;

	/* Calculate CRC of entire packet. */
	crc = mcumgr_dummy_calc_crc(data, len);

	/* Transmit packet as a sequence of frames. */
	src_off = 0;
	while (src_off < len) {
		rc = mcumgr_dummy_tx_frame(data + src_off,
					   src_off == 0,
					   len - src_off,
					   crc, cb,
					   &data_bytes_txed);
		if (rc != 0) {
			return rc;
		}

		src_off += data_bytes_txed;
	}

	return 0;
}

static int smp_receive(const void *data, int len)
{
	uint16_t data_size =
		MIN(len, (sizeof(smp_receive_buffer) - smp_receive_pos - 1));

	if (enable_dummy_smp == true) {
		memcpy(&smp_receive_buffer[smp_receive_pos], data, data_size);
		smp_receive_pos += data_size;
	}

	return 0;
}

bool smp_dummy_wait_for_data(uint32_t wait_time_s)
{
	return k_sem_take(&smp_data_ready_sem, K_SECONDS(wait_time_s)) == 0 ? true : false;
}

void smp_dummy_add_data(void)
{
	dummy_mcumgr_add_data(smp_receive_buffer, smp_receive_pos);
}

uint16_t smp_dummy_get_send_pos(void)
{
	return smp_send_pos;
}

uint16_t smp_dummy_get_receive_pos(void)
{
	return smp_receive_pos;
}

int smp_dummy_tx_pkt(const uint8_t *data, int len)
{
	return mcumgr_dummy_tx_pkt(data, len, smp_receive);
}

void smp_dummy_enable(void)
{
	enable_dummy_smp = true;
}

void smp_dummy_disable(void)
{
	enable_dummy_smp = false;
}

bool smp_dummy_get_status(void)
{
	return enable_dummy_smp;
}

SYS_INIT(smp_dummy_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
