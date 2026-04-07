/*
 * Copyright (c) 2017 Linaro, Ltd.
 * Copyright (c) 2026 Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief SPI transport for the mcumgr SMP protocol.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/logging/log.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

LOG_MODULE_DECLARE(mcumgr_smp, CONFIG_MCUMGR_TRANSPORT_LOG_LEVEL);

#define SMP_SPI_NODE DT_NODELABEL(smp_spi)

#if !DT_NODE_HAS_STATUS(SMP_SPI_NODE, okay)
#error "smp_spi devicetree node is missing or disabled"
#endif

static const struct device *const spi_dev = DEVICE_DT_GET(DT_PHANDLE(SMP_SPI_NODE, spi_dev));

#define SPI_BUF_SIZE CONFIG_MCUMGR_TRANSPORT_SPI_MTU
#define SPI_MODE_FLAGS (SPI_MODE_CPOL | SPI_MODE_CPHA)

#define HDR_MAGIC0 0xA5
#define HDR_MAGIC1 0x5A
#define HDR_SIZE   10
#define PAYLOAD_MAX (SPI_BUF_SIZE - HDR_SIZE)

#define FLAG_SYNC  0x01
#define FLAG_ACK   0x02
#define FLAG_DATA  0x04
#define FLAG_POLL  0x08
#define FLAG_RESET 0x10

#define SMP_HDR_SIZE 8
#define SEG_HDR_SIZE 6 /* u16 total, u16 offset, u8 flags, u8 rsv */
#define SEG_FLAG_LAST 0x01

#define SMP_BUF_MAX (PAYLOAD_MAX * 2)

#if defined(CONFIG_MCUMGR_TRANSPORT_SPI_REASSEMBLY)
#define SMP_REASSEMBLY_TIMEOUT_MS CONFIG_MCUMGR_TRANSPORT_SPI_REASSEMBLY_TIMEOUT_MS
#else
#define SMP_REASSEMBLY_TIMEOUT_MS 0
#endif

#if defined(CONFIG_MCUMGR_TRANSPORT_SPI_DEBUG)
#define SPI_DBG(...) LOG_DBG(__VA_ARGS__)
#else
#define SPI_DBG(...) do { } while (0)
#endif

static uint8_t smp_buf[SMP_BUF_MAX];
static uint16_t smp_expected_len;
static uint16_t smp_filled;
static int64_t smp_last_seg_ms;

static uint8_t rx_buf[SPI_BUF_SIZE];
static uint8_t tx_buf[SPI_BUF_SIZE];
static uint8_t seg_payload[PAYLOAD_MAX];

static uint8_t smp_tx_buf[SMP_BUF_MAX];
static uint16_t smp_tx_len;
static uint16_t smp_tx_offset;
static bool smp_tx_ready;
static bool tx_seg_pending;
static uint16_t tx_seg_chunk;
static bool tx_seg_last;

static struct smp_transport smp_spi_transport;

static K_THREAD_STACK_DEFINE(smp_spi_stack,
			     CONFIG_MCUMGR_TRANSPORT_SPI_RX_THREAD_STACK_SIZE);
static struct k_thread smp_spi_thread_data;

static const struct spi_config smp_spi_cfg = {
	.operation = SPI_OP_MODE_SLAVE | SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		     SPI_MODE_FLAGS,
	.frequency = 50000U,
	.slave = CONFIG_MCUMGR_TRANSPORT_SPI_SLAVE,
};

static uint32_t smp_spi_crc32(uint8_t flags, uint8_t seq,
			      uint16_t payload_len,
			      const uint8_t *payload);

static void smp_spi_dump_smp_header(const uint8_t *smp);

static void build_frame(uint8_t flags, uint8_t seq, const uint8_t *payload,
			uint16_t payload_len);

static void smp_spi_reset_rx_state(void)
{
	smp_expected_len = 0;
	smp_filled = 0;
}

static void smp_spi_reset_tx_state(void)
{
	smp_tx_ready = false;
	smp_tx_len = 0;
	smp_tx_offset = 0;
	tx_seg_pending = false;
}

static void smp_spi_reset_all_state(void)
{
	smp_spi_reset_rx_state();
	smp_spi_reset_tx_state();
}

static void smp_spi_finish_tx_segment(void)
{
	if (!tx_seg_pending) {
		return;
	}

	smp_tx_offset = (uint16_t)(smp_tx_offset + tx_seg_chunk);
	if (tx_seg_last || smp_tx_offset >= smp_tx_len) {
		smp_tx_ready = false;
	}
	tx_seg_pending = false;
}

static uint32_t smp_spi_crc32(uint8_t flags, uint8_t seq,
			      uint16_t payload_len,
			      const uint8_t *payload)
{
	uint8_t hdr_bytes[4];
	uint32_t crc;

	hdr_bytes[0] = flags;
	hdr_bytes[1] = seq;
	hdr_bytes[2] = (uint8_t)(payload_len & 0xFF);
	hdr_bytes[3] = (uint8_t)((payload_len >> 8) & 0xFF);

	crc = crc32_ieee_update(0U, hdr_bytes, sizeof(hdr_bytes));

	if ((payload_len > 0U) && (payload != NULL)) {
		crc = crc32_ieee_update(crc, payload, payload_len);
	}

	return crc;
}

static bool smp_spi_validate_frame(uint8_t flags, uint8_t seq, uint16_t *len)
{
	uint32_t crc_rx;
	uint32_t crc_calc;

	if (*len > PAYLOAD_MAX) {
		*len = PAYLOAD_MAX;
	}

	crc_rx = (uint32_t)rx_buf[6] |
		 ((uint32_t)rx_buf[7] << 8) |
		 ((uint32_t)rx_buf[8] << 16) |
		 ((uint32_t)rx_buf[9] << 24);
	crc_calc = smp_spi_crc32(flags, seq, *len, rx_buf + HDR_SIZE);

	if (crc_calc == crc_rx) {
		return true;
	}

	LOG_WRN("SPI bad-crc flags=%02x seq=%u len=%u", flags, seq, *len);
	if (smp_expected_len > 0) {
		SPI_DBG("SMP reassembly reset due to CRC error");
		smp_spi_reset_rx_state();
	}

	return false;
}

static void smp_spi_submit_packet(const uint8_t *buf, uint16_t len)
{
	struct net_buf *nb = smp_packet_alloc();

	if (nb == NULL) {
		LOG_ERR("SMP packet alloc failed");
		return;
	}

	if (len > net_buf_tailroom(nb)) {
		LOG_ERR("SMP buf too large for net_buf: %u", len);
		smp_packet_free(nb);
		return;
	}

	net_buf_add_mem(nb, buf, len);
	SPI_DBG("SMP rx submit len=%u", len);
	smp_rx_req(&smp_spi_transport, nb);
}

static void smp_spi_handle_reset(uint8_t seq)
{
	SPI_DBG("SPI reset seq=%u", seq);
	smp_spi_reset_all_state();
	build_frame(FLAG_ACK, seq, NULL, 0);
}

static void smp_spi_handle_poll(uint8_t seq)
{
	SPI_DBG("SPI poll seq=%u", seq);
	if (!smp_tx_ready && !tx_seg_pending) {
		build_frame(FLAG_ACK, seq, NULL, 0);
	}
}

static void smp_spi_handle_sync(uint8_t seq)
{
	SPI_DBG("SPI sync seq=%u", seq);
	smp_spi_reset_tx_state();
	build_frame(FLAG_ACK, seq, NULL, 0);
}

static void smp_spi_handle_data_no_reassembly(uint8_t seq, uint16_t len)
{
	if (len < SMP_HDR_SIZE) {
		build_frame(FLAG_ACK, seq, NULL, 0);
		return;
	}

	smp_spi_submit_packet(rx_buf + HDR_SIZE, len);

	if (!tx_seg_pending) {
		build_frame(FLAG_ACK, seq, NULL, 0);
	}
}

static bool smp_spi_segment_too_large(uint16_t total, uint8_t seq)
{
	if (total <= sizeof(smp_buf)) {
		return false;
	}

	LOG_WRN("SMP total too large: %u", total);
	build_frame(FLAG_ACK, seq, NULL, 0);
	return true;
}

static void smp_spi_store_segment(uint16_t offset, uint16_t data_len, int64_t now_ms)
{
	if (offset + data_len <= sizeof(smp_buf)) {
		memcpy(&smp_buf[offset], &rx_buf[HDR_SIZE + SEG_HDR_SIZE], data_len);
		smp_filled += data_len;
		smp_last_seg_ms = now_ms;
	}
}

static void smp_spi_complete_reassembly(void)
{
	if (smp_expected_len >= SMP_HDR_SIZE) {
		smp_spi_dump_smp_header(smp_buf);
	}

	SPI_DBG("SMP reassembly done len=%u", smp_expected_len);
	smp_spi_submit_packet(smp_buf, smp_expected_len);
	smp_spi_reset_rx_state();
}

static void smp_spi_handle_segmented_data(uint8_t seq, uint16_t len, int64_t now_ms)
{
	uint16_t total;
	uint16_t offset;
	uint8_t sflags;
	uint16_t data_len;

	if (len < SEG_HDR_SIZE) {
		if (!tx_seg_pending) {
			build_frame(FLAG_ACK, seq, NULL, 0);
		}
		return;
	}

	total = (uint16_t)rx_buf[HDR_SIZE] |
		((uint16_t)rx_buf[HDR_SIZE + 1] << 8);
	offset = (uint16_t)rx_buf[HDR_SIZE + 2] |
		 ((uint16_t)rx_buf[HDR_SIZE + 3] << 8);
	sflags = rx_buf[HDR_SIZE + 4];
	data_len = len - SEG_HDR_SIZE;

	SPI_DBG("SEG total=%u offset=%u data_len=%u last=%u",
		total, offset, data_len, (sflags & SEG_FLAG_LAST) ? 1 : 0);

	if (smp_spi_segment_too_large(total, seq)) {
		return;
	}

	if (offset == 0) {
		smp_expected_len = total;
		smp_filled = 0;
		smp_last_seg_ms = now_ms;
	}

	smp_spi_store_segment(offset, data_len, now_ms);

	if ((sflags & SEG_FLAG_LAST) &&
	    smp_expected_len > 0 &&
	    smp_expected_len <= sizeof(smp_buf)) {
		smp_spi_complete_reassembly();
	}

	if (!tx_seg_pending) {
		build_frame(FLAG_ACK, seq, NULL, 0);
	}
}

static void smp_spi_handle_data(uint8_t seq, uint16_t len, int64_t now_ms)
{
	SPI_DBG("SPI data seq=%u len=%u", seq, len);

#if !defined(CONFIG_MCUMGR_TRANSPORT_SPI_REASSEMBLY)
	smp_spi_handle_data_no_reassembly(seq, len);
#else
	smp_spi_handle_segmented_data(seq, len, now_ms);
#endif
}

static void smp_spi_handle_other(uint8_t flags, uint8_t seq, uint16_t len)
{
	SPI_DBG("SPI frame flags=%02x seq=%u len=%u", flags, seq, len);
	if (!tx_seg_pending) {
		build_frame(FLAG_ACK, seq, NULL, 0);
	}
}

static void smp_spi_process_frame(uint8_t flags, uint8_t seq, uint16_t len, int64_t now_ms)
{
	if (flags == FLAG_ACK) {
		return;
	}

	if (flags & FLAG_RESET) {
		smp_spi_handle_reset(seq);
		return;
	}

	if (flags & FLAG_POLL) {
		smp_spi_handle_poll(seq);
		return;
	}

	if (flags & FLAG_SYNC) {
		smp_spi_handle_sync(seq);
		return;
	}

	if (flags & FLAG_DATA) {
		smp_spi_handle_data(seq, len, now_ms);
		return;
	}

	smp_spi_handle_other(flags, seq, len);
}

static int smp_spi_tx_pkt(struct net_buf *nb)
{
	uint16_t len = nb->len;

	if (len > sizeof(smp_tx_buf)) {
		len = sizeof(smp_tx_buf);
	}
	memcpy(smp_tx_buf, nb->data, len);
	smp_tx_len = len;
	smp_tx_offset = 0;
	smp_tx_ready = true;
	SPI_DBG("SMP tx queued len=%u", len);
	smp_packet_free(nb);
	return 0;
}

static void prepare_tx_segment(uint8_t seq)
{
	if (tx_seg_pending) {
		return;
	}

	if (!(smp_tx_ready && smp_tx_offset < smp_tx_len)) {
		return;
	}

	uint16_t total = smp_tx_len;
	uint16_t offset = smp_tx_offset;
	uint16_t chunk = total - offset;
	uint16_t max_chunk = PAYLOAD_MAX - SEG_HDR_SIZE;

	if (chunk > max_chunk) {
		chunk = max_chunk;
	}

	uint8_t seg_flags = (offset + chunk >= total) ? SEG_FLAG_LAST : 0;

	seg_payload[0] = (uint8_t)(total & 0xFF);
	seg_payload[1] = (uint8_t)((total >> 8) & 0xFF);
	seg_payload[2] = (uint8_t)(offset & 0xFF);
	seg_payload[3] = (uint8_t)((offset >> 8) & 0xFF);
	seg_payload[4] = seg_flags;
	seg_payload[5] = 0;
	memcpy(&seg_payload[SEG_HDR_SIZE], &smp_tx_buf[offset], chunk);

	build_frame(FLAG_DATA, seq, seg_payload, (uint16_t)(SEG_HDR_SIZE + chunk));

	SPI_DBG("SMP tx seg offset=%u len=%u last=%u",
	       offset, chunk, (seg_flags & SEG_FLAG_LAST) ? 1 : 0);

	tx_seg_pending = true;
	tx_seg_chunk = chunk;
	tx_seg_last = (seg_flags & SEG_FLAG_LAST) ? true : false;
}

static void smp_spi_dump_smp_header(const uint8_t *smp)
{
#if defined(CONFIG_MCUMGR_TRANSPORT_SPI_DEBUG)
	uint8_t op = smp[0];
	uint8_t fl = smp[1];
	uint16_t slen = ((uint16_t)smp[2] << 8) | smp[3];
	uint16_t sgroup = ((uint16_t)smp[4] << 8) | smp[5];
	uint8_t sseq = smp[6];
	uint8_t sid = smp[7];
	uint8_t op_dec = op & 0x07;
	uint8_t ver_dec = (op >> 3) & 0x03;
	const struct mgmt_group *grp = mgmt_find_group(sgroup);
	const struct mgmt_handler *h = grp ? mgmt_get_handler(grp, sid) : NULL;

	SPI_DBG("SMP raw hdr bytes: %02x %02x %02x %02x %02x %02x %02x %02x",
		smp[0], smp[1], smp[2], smp[3],
		smp[4], smp[5], smp[6], smp[7]);
	SPI_DBG("SMP hdr op=0x%02x (op=%u ver=%u) flags=%02x len=%u group=%u seq=%u id=%u",
		op, op_dec, ver_dec, fl, slen, sgroup, sseq, sid);
	SPI_DBG("MCUMGR lookup group=%u found handler=%s read=%d write=%d",
		sgroup, h ? "yes" : "no", h ? h->mh_read != NULL : 0,
		h ? h->mh_write != NULL : 0);
#else
	ARG_UNUSED(smp);
#endif
}

static uint16_t smp_spi_get_mtu(const struct net_buf *nb)
{
	ARG_UNUSED(nb);
	return SMP_BUF_MAX;
}

static void build_frame(uint8_t flags, uint8_t seq, const uint8_t *payload,
			uint16_t payload_len)
{
	uint32_t crc;

	if (payload_len > PAYLOAD_MAX) {
		payload_len = PAYLOAD_MAX;
	}

	memset(tx_buf, 0, sizeof(tx_buf));
	tx_buf[0] = HDR_MAGIC0;
	tx_buf[1] = HDR_MAGIC1;
	tx_buf[2] = flags;
	tx_buf[3] = seq;
	tx_buf[4] = (uint8_t)(payload_len & 0xFF);
	tx_buf[5] = (uint8_t)((payload_len >> 8) & 0xFF);

	if ((payload_len > 0U) && (payload != NULL)) {
		memcpy(tx_buf + HDR_SIZE, payload, payload_len);
	}

	crc = smp_spi_crc32(flags, seq, payload_len, payload);

	tx_buf[6] = (uint8_t)(crc & 0xFF);
	tx_buf[7] = (uint8_t)((crc >> 8) & 0xFF);
	tx_buf[8] = (uint8_t)((crc >> 16) & 0xFF);
	tx_buf[9] = (uint8_t)((crc >> 24) & 0xFF);
}

static void smp_spi_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct spi_buf rx = {
		.buf = rx_buf,
		.len = sizeof(rx_buf),
	};
	struct spi_buf tx = {
		.buf = tx_buf,
		.len = sizeof(tx_buf),
	};
	const struct spi_buf_set rx_set = {
		.buffers = &rx,
		.count = 1,
	};
	const struct spi_buf_set tx_set = {
		.buffers = &tx,
		.count = 1,
	};

	while (1) {
		int ret;
		int64_t now_ms = k_uptime_get();

#if defined(CONFIG_MCUMGR_TRANSPORT_SPI_REASSEMBLY)
		if (smp_expected_len > 0 &&
		    (now_ms - smp_last_seg_ms) > SMP_REASSEMBLY_TIMEOUT_MS) {
			SPI_DBG("SMP reassembly timeout; dropping partial message");
			smp_expected_len = 0;
			smp_filled = 0;
		}
#endif

		memset(rx_buf, 0, sizeof(rx_buf));
		ret = spi_transceive(spi_dev, &smp_spi_cfg, &tx_set, &rx_set);

		if (ret < 0) {
			LOG_ERR("spi_transceive failed: %d", ret);
			k_sleep(K_MSEC(100));
			continue;
		}

		smp_spi_finish_tx_segment();

		if (rx_buf[0] != HDR_MAGIC0 || rx_buf[1] != HDR_MAGIC1) {
			continue;
		}

		uint8_t flags = rx_buf[2];
		uint8_t seq = rx_buf[3];
		uint16_t len = (uint16_t)rx_buf[4] | ((uint16_t)rx_buf[5] << 8);

		if (!smp_spi_validate_frame(flags, seq, &len)) {
			continue;
		}

		smp_spi_process_frame(flags, seq, len, now_ms);
		prepare_tx_segment(seq);
	}
}

static int smp_spi_init(void)
{
	int rc;

	if (!device_is_ready(spi_dev)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	smp_spi_transport.functions.output = smp_spi_tx_pkt;
	smp_spi_transport.functions.get_mtu = smp_spi_get_mtu;

	rc = smp_transport_init(&smp_spi_transport);
	if (rc != 0) {
		LOG_ERR("SPI SMP transport init failed: %d", rc);
		return rc;
	}

	build_frame(FLAG_ACK, 0, NULL, 0);

	LOG_INF("SPI SMP transport initialized");

	k_thread_create(&smp_spi_thread_data, smp_spi_stack,
			K_THREAD_STACK_SIZEOF(smp_spi_stack),
			smp_spi_thread, NULL, NULL, NULL,
			CONFIG_MCUMGR_TRANSPORT_SPI_RX_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&smp_spi_thread_data, "smp_spi");

#ifdef CONFIG_SMP_CLIENT
	static struct smp_client_transport_entry smp_spi_client_transport;

	smp_spi_client_transport.smpt = &smp_spi_transport;
	smp_spi_client_transport.smpt_type = SMP_SPI_TRANSPORT;
	smp_client_transport_register(&smp_spi_client_transport);
#endif

	return 0;
}

SYS_INIT(smp_spi_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
