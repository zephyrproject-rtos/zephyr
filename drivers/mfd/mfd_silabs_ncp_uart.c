/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Silicon Labs network co-processor over UART: CPC primary transport.
 *
 * Wire protocol (see sl_cpc_drv_uart.c on the secondary side):
 *
 *  - A frame is a preamble byte (0xEB), the 8-byte CPC header and the CRC-16
 *    (XMODEM, little endian) of the header. When the header announces a
 *    payload, the payload follows, ended by its own CRC-16. The receiver
 *    synchronizes on the preamble and the header CRC, sliding one byte at a
 *    time over anything else.
 *  - There is no flow control at the CPC level: the UART hardware flow
 *    control, when wired, holds the co-processor back.
 *
 * The transport uses the asynchronous UART API. The UART callback empties the
 * DMA buffers into a ring buffer; the frames are parsed and sent from the
 * system work queue. The speed and the flow control of the link come from the
 * devicetree of the UART controller.
 */

#define DT_DRV_COMPAT silabs_ncp

#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/section_tags.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mfd_silabs_ncp_uart, CONFIG_MFD_LOG_LEVEL);

#include "mfd_silabs_ncp.h"
#include "sli_cpc_crc.h"

#define NCP_UART_PREAMBLE       0xEB
#define NCP_UART_CRC_SIZE       sizeof(uint16_t)
#define NCP_UART_HDR_BLOCK_SIZE (1 + SLI_CPC_HEADER_SIZE + NCP_UART_CRC_SIZE)
#define NCP_UART_MAX_FRAME                                                                         \
	(NCP_UART_HDR_BLOCK_SIZE + SL_CPC_EP_MAX_PAYLOAD_SIZE + NCP_UART_CRC_SIZE)
/* Idle time on the line after which the UART reports what it received */
#define NCP_UART_RX_TIMEOUT_US  1000
#define NCP_UART_RETRY_DELAY    K_MSEC(10)

/* Events raised by the UART callback, handled by the work queue */
enum ncp_uart_event {
	NCP_UART_EVENT_TX_DONE,
	NCP_UART_EVENT_TX_ABORTED,
	NCP_UART_EVENT_RX_OVERRUN,
	NCP_UART_EVENT_RX_DISABLED,
};

enum ncp_uart_rx_state {
	NCP_UART_RX_HEADER,
	NCP_UART_RX_PAYLOAD,
	NCP_UART_RX_CRC,
	/* Skipping the payload of a frame that could not be allocated */
	NCP_UART_RX_FLUSH,
};

struct ncp_uart_config {
	const struct device *uart;
	struct gpio_dt_spec wake;
	struct gpio_dt_spec reset;
	/* NCP_UART_MAX_FRAME bytes, usable by the UART DMA */
	uint8_t *tx_buf;
	/* Two reception buffers, usable by the UART DMA */
	uint8_t *rx_bufs[2];
	size_t rx_buf_size;
};

struct ncp_uart_data {
	struct mfd_silabs_ncp_data common;
	const struct device *dev;
	struct k_work_delayable work;
	atomic_t events;
	/* Bit i is set while rx_bufs[i] is owned by the UART */
	atomic_t rx_bufs_used;
	bool rx_enabled;
	bool running;
	struct ring_buf rx_rb;
	uint8_t rx_rb_buf[CONFIG_MFD_SILABS_NCP_UART_RX_RING_BUF_SIZE];
	/* Reception state machine */
	enum ncp_uart_rx_state rx_state;
	sl_cpc_frame_t *rx_frame;
	/* Buffer of the payload chain being filled, and position in it */
	sl_cpc_buf_t *rx_chain;
	uint16_t rx_chain_off;
	/* Payload length (or number of bytes to flush) and bytes already handled */
	uint16_t rx_len;
	uint16_t rx_pos;
	uint16_t rx_crc;
	/* Frame on the wire */
	sl_cpc_frame_t *tx_frame;
	/* Frames submitted by the core, waiting for transmission */
	sli_cpc_frame_list_t tx_queue;
	/* Frames sent on the wire, waiting to be handed back to the core */
	sli_cpc_frame_list_t tx_done;
	/* Pre-allocated RX frames */
	sli_cpc_frame_list_t rx_free;
	/* Received frames, waiting to be read by the core */
	sli_cpc_frame_list_t rx_pending;
};

static inline struct ncp_uart_data *ncp_uart_data_from_bus(sl_cpc_bus_t *bus)
{
	return CONTAINER_OF(bus, struct ncp_uart_data, common.cpc_bus);
}

static void ncp_uart_wakeup(struct ncp_uart_data *data)
{
	k_work_reschedule(&data->work, K_NO_WAIT);
}

static void ncp_uart_callback(const struct device *uart, struct uart_event *evt, void *user_data)
{
	const struct device *dev = user_data;
	const struct ncp_uart_config *cfg = dev->config;
	struct ncp_uart_data *data = dev->data;

	switch (evt->type) {
	case UART_TX_DONE:
		atomic_set_bit(&data->events, NCP_UART_EVENT_TX_DONE);
		ncp_uart_wakeup(data);
		break;
	case UART_TX_ABORTED:
		atomic_set_bit(&data->events, NCP_UART_EVENT_TX_ABORTED);
		ncp_uart_wakeup(data);
		break;
	case UART_RX_BUF_REQUEST:
		for (int i = 0; i < ARRAY_SIZE(cfg->rx_bufs); i++) {
			if (!atomic_test_and_set_bit(&data->rx_bufs_used, i)) {
				uart_rx_buf_rsp(uart, cfg->rx_bufs[i], cfg->rx_buf_size);
				return;
			}
		}
		LOG_WRN("%s: no free RX buffer", dev->name);
		break;
	case UART_RX_BUF_RELEASED:
		for (int i = 0; i < ARRAY_SIZE(cfg->rx_bufs); i++) {
			if (evt->data.rx_buf.buf == cfg->rx_bufs[i]) {
				atomic_clear_bit(&data->rx_bufs_used, i);
			}
		}
		break;
	case UART_RX_RDY:
		if (ring_buf_put(&data->rx_rb, &evt->data.rx.buf[evt->data.rx.offset],
				 evt->data.rx.len) < evt->data.rx.len) {
			atomic_set_bit(&data->events, NCP_UART_EVENT_RX_OVERRUN);
		}
		ncp_uart_wakeup(data);
		break;
	case UART_RX_DISABLED:
		atomic_set_bit(&data->events, NCP_UART_EVENT_RX_DISABLED);
		ncp_uart_wakeup(data);
		break;
	case UART_RX_STOPPED:
		LOG_WRN("%s: reception error: %d", dev->name, evt->data.rx_stop.reason);
		break;
	default:
		break;
	}
}

static int ncp_uart_rx_enable(const struct device *dev)
{
	const struct ncp_uart_config *cfg = dev->config;
	struct ncp_uart_data *data = dev->data;
	int ret;

	atomic_set(&data->rx_bufs_used, BIT(0));
	ret = uart_rx_enable(cfg->uart, cfg->rx_bufs[0], cfg->rx_buf_size,
			     NCP_UART_RX_TIMEOUT_US);
	if (ret < 0) {
		LOG_ERR("%s: cannot enable reception: %d", dev->name, ret);
		atomic_clear(&data->rx_bufs_used);
		return ret;
	}
	data->rx_enabled = true;

	return 0;
}

/* Drop the frame being received and wait for the next header */
static void ncp_uart_rx_reset(struct ncp_uart_data *data)
{
	if (data->rx_frame) {
		mfd_silabs_ncp_drop_rx_frame(data->rx_frame);
		data->rx_frame = NULL;
	}
	data->rx_state = NCP_UART_RX_HEADER;
}

/* Hand the frame being received to the core */
static void ncp_uart_rx_deliver(struct ncp_uart_data *data)
{
	unsigned int key;

	key = irq_lock();
	sli_cpc_frame_list_push_back(&data->rx_pending, data->rx_frame);
	irq_unlock(key);
	data->rx_frame = NULL;
	data->rx_state = NCP_UART_RX_HEADER;
	sli_cpc_bus_notify_rx_data_from_drv(&data->common.cpc_bus);
}

/* Look for a header block at the head of the ring buffer. Returns true when
 * bytes were consumed.
 */
static bool ncp_uart_rx_header(const struct device *dev)
{
	struct ncp_uart_data *data = dev->data;
	uint8_t blk[NCP_UART_HDR_BLOCK_SIZE];
	sl_cpc_frame_t *frame;
	sli_cpc_hdr_t *hdr;
	uint16_t payload_len;
	unsigned int key;
	int status;

	if (ring_buf_peek(&data->rx_rb, blk, sizeof(blk)) < sizeof(blk)) {
		return false;
	}
	if (blk[0] != NCP_UART_PREAMBLE ||
	    sli_cpc_get_crc_sw(&blk[1], SLI_CPC_HEADER_SIZE) !=
		    sys_get_le16(&blk[1 + SLI_CPC_HEADER_SIZE])) {
		/* Not the start of a frame, slide by one byte */
		ring_buf_get(&data->rx_rb, blk, 1);
		return true;
	}

	key = irq_lock();
	frame = sli_cpc_frame_list_pop(&data->rx_free);
	irq_unlock(key);
	if (!frame) {
		/* Wait for the core to release a frame */
		return false;
	}
	ring_buf_get(&data->rx_rb, blk, sizeof(blk));
	LOG_HEXDUMP_DBG(blk, sizeof(blk), "rx header");

	hdr = sli_cpc_frame_get_header(frame);
	memcpy(hdr, &blk[1], SLI_CPC_HEADER_SIZE);
	if (!sli_cpc_header_is_size_valid(hdr)) {
		LOG_ERR("%s: invalid payload size", dev->name);
		key = irq_lock();
		sli_cpc_frame_list_push_back(&data->rx_free, frame);
		irq_unlock(key);
		return true;
	}
	payload_len = sli_cpc_header_get_payload_size(hdr);

	/* Bind the endpoint and allocate the payload chain */
	status = sli_cpc_alloc_rx_payload(&data->common.cpc_bus, frame);
	if (status) {
		LOG_WRN("%s: cannot allocate RX payload: %d", dev->name, status);
		mfd_silabs_ncp_drop_rx_frame(frame);
		if (payload_len > 0) {
			data->rx_len = payload_len + NCP_UART_CRC_SIZE;
			data->rx_pos = 0;
			data->rx_state = NCP_UART_RX_FLUSH;
		}
		return true;
	}

	data->rx_frame = frame;
	if (payload_len == 0) {
		ncp_uart_rx_deliver(data);
		return true;
	}
	data->rx_chain = frame->payload;
	data->rx_chain_off = 0;
	data->rx_len = payload_len;
	data->rx_pos = 0;
	data->rx_crc = 0;
	data->rx_state = NCP_UART_RX_PAYLOAD;

	return true;
}

/* Move payload bytes from the ring buffer into the payload chain */
static bool ncp_uart_rx_payload(const struct device *dev)
{
	struct ncp_uart_data *data = dev->data;
	sl_cpc_buf_t *buf = data->rx_chain;
	uint8_t *dst;
	uint32_t len;

	if (!buf) {
		LOG_ERR("%s: payload chain shorter than the payload", dev->name);
		ncp_uart_rx_reset(data);
		return true;
	}
	dst = (uint8_t *)buf->ptr + data->rx_chain_off;
	len = MIN(buf->len - data->rx_chain_off, data->rx_len - data->rx_pos);
	len = ring_buf_get(&data->rx_rb, dst, len);
	if (!len) {
		return false;
	}
	LOG_HEXDUMP_DBG(dst, len, "rx payload");
	data->rx_crc = crc16_itu_t(data->rx_crc, dst, len);
	data->rx_pos += len;
	data->rx_chain_off += len;
	if (data->rx_chain_off == buf->len) {
		data->rx_chain = (sl_cpc_buf_t *)buf->node.node;
		data->rx_chain_off = 0;
	}
	if (data->rx_pos == data->rx_len) {
		data->rx_state = NCP_UART_RX_CRC;
	}

	return true;
}

static bool ncp_uart_rx_crc(const struct device *dev)
{
	struct ncp_uart_data *data = dev->data;
	sl_cpc_frame_t *frame = data->rx_frame;
	uint8_t crc_le[NCP_UART_CRC_SIZE];

	if (ring_buf_size_get(&data->rx_rb) < sizeof(crc_le)) {
		return false;
	}
	ring_buf_get(&data->rx_rb, crc_le, sizeof(crc_le));
	frame->payload_csum = sys_get_le16(crc_le);
	frame->payload_csum_is_valid = frame->payload_csum == data->rx_crc;
	if (!frame->payload_csum_is_valid) {
		LOG_WRN("%s: payload CRC mismatch: 0x%04x, expected 0x%04x", dev->name,
			frame->payload_csum, data->rx_crc);
	}
	ncp_uart_rx_deliver(data);

	return true;
}

/* Skip the payload and the CRC of a frame that was not allocated */
static bool ncp_uart_rx_flush(const struct device *dev)
{
	struct ncp_uart_data *data = dev->data;
	uint8_t *ptr;
	uint32_t len;

	len = ring_buf_get_ptr(&data->rx_rb, &ptr, 0);
	len = MIN(len, data->rx_len - data->rx_pos);
	if (!len) {
		return false;
	}
	ring_buf_consume(&data->rx_rb, len);
	data->rx_pos += len;
	if (data->rx_pos == data->rx_len) {
		data->rx_state = NCP_UART_RX_HEADER;
	}

	return true;
}

/* Run one step of the reception state machine. Returns true when progress
 * was made.
 */
static bool ncp_uart_rx_step(const struct device *dev)
{
	struct ncp_uart_data *data = dev->data;

	switch (data->rx_state) {
	case NCP_UART_RX_HEADER:
		return ncp_uart_rx_header(dev);
	case NCP_UART_RX_PAYLOAD:
		return ncp_uart_rx_payload(dev);
	case NCP_UART_RX_CRC:
		return ncp_uart_rx_crc(dev);
	case NCP_UART_RX_FLUSH:
		return ncp_uart_rx_flush(dev);
	default:
		return false;
	}
}

/* Hand back the frame that was on the wire */
static void ncp_uart_tx_complete(const struct device *dev)
{
	struct ncp_uart_data *data = dev->data;
	bool done = atomic_test_and_clear_bit(&data->events, NCP_UART_EVENT_TX_DONE);
	bool aborted = atomic_test_and_clear_bit(&data->events, NCP_UART_EVENT_TX_ABORTED);
	unsigned int key;

	if (!data->tx_frame || (!done && !aborted)) {
		return;
	}
	if (aborted) {
		LOG_WRN("%s: transmission aborted, retrying", dev->name);
		key = irq_lock();
		sli_cpc_frame_list_push_front(&data->tx_queue, data->tx_frame);
		irq_unlock(key);
		data->tx_frame = NULL;
		return;
	}
	key = irq_lock();
	sli_cpc_frame_list_push_back(&data->tx_done, data->tx_frame);
	irq_unlock(key);
	data->tx_frame = NULL;
	sli_cpc_bus_notify_tx_data_by_drv(&data->common.cpc_bus, &data->tx_done);
}

/* Put the next frame on the wire, if none is in flight */
static void ncp_uart_tx(const struct device *dev)
{
	const struct ncp_uart_config *cfg = dev->config;
	struct ncp_uart_data *data = dev->data;
	uint8_t *buf = cfg->tx_buf;
	sl_cpc_frame_t *frame;
	sli_cpc_hdr_t *hdr;
	size_t len = NCP_UART_HDR_BLOCK_SIZE;
	uint16_t payload_len;
	uint16_t crc;
	unsigned int key;
	int ret;

	if (data->tx_frame) {
		return;
	}
	key = irq_lock();
	frame = sli_cpc_frame_list_pop(&data->tx_queue);
	irq_unlock(key);
	if (!frame) {
		return;
	}

	hdr = sli_cpc_frame_get_header(frame);
	buf[0] = NCP_UART_PREAMBLE;
	memcpy(&buf[1], hdr, SLI_CPC_HEADER_SIZE);
	crc = sli_cpc_get_crc_sw(&buf[1], SLI_CPC_HEADER_SIZE);
	sys_put_le16(crc, &buf[1 + SLI_CPC_HEADER_SIZE]);

	payload_len = sli_cpc_header_get_payload_size(hdr);
	if (payload_len > 0) {
		mfd_silabs_ncp_copy_from_chain(&buf[len], frame->payload, payload_len);
		crc = sli_cpc_get_crc_sw(&buf[len], payload_len);
		len += payload_len;
		sys_put_le16(crc, &buf[len]);
		len += NCP_UART_CRC_SIZE;
		frame->payload_csum = crc;
		frame->payload_csum_is_valid = true;
	}

	LOG_HEXDUMP_DBG(buf, len, "tx");
	data->tx_frame = frame;
	ret = uart_tx(cfg->uart, buf, len, SYS_FOREVER_US);
	LOG_DBG("%s: uart_tx(%zu bytes): %d", dev->name, len, ret);
	if (ret < 0) {
		LOG_ERR("%s: transmission failed: %d", dev->name, ret);
		data->tx_frame = NULL;
		key = irq_lock();
		sli_cpc_frame_list_push_front(&data->tx_queue, frame);
		irq_unlock(key);
		k_work_reschedule(&data->work, NCP_UART_RETRY_DELAY);
	}
}

static void ncp_uart_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ncp_uart_data *data = CONTAINER_OF(dwork, struct ncp_uart_data, work);
	const struct device *dev = data->dev;

	if (atomic_test_and_clear_bit(&data->events, NCP_UART_EVENT_RX_DISABLED)) {
		data->rx_enabled = false;
		if (data->running) {
			LOG_WRN("%s: reception stopped, restarting", dev->name);
			if (ncp_uart_rx_enable(dev) < 0) {
				k_work_reschedule(&data->work, NCP_UART_RETRY_DELAY);
			}
		}
	}
	if (!data->running) {
		return;
	}
	if (atomic_test_and_clear_bit(&data->events, NCP_UART_EVENT_RX_OVERRUN)) {
		LOG_WRN("%s: RX ring buffer overrun", dev->name);
		ncp_uart_rx_reset(data);
	}

	ncp_uart_tx_complete(dev);
	while (ncp_uart_rx_step(dev)) {
	}
	ncp_uart_tx(dev);
}

/* Keep the pool of RX frames full. Called from the core when a RX frame is
 * released, and from the transport itself.
 */
static void ncp_uart_refill_rx(const struct device *dev)
{
	struct ncp_uart_data *data = dev->data;
	sl_cpc_frame_t *frame;
	unsigned int key;
	bool was_empty;

	key = irq_lock();
	was_empty = sli_cpc_frame_list_empty(&data->rx_free);
	irq_unlock(key);

	while (true) {
		key = irq_lock();
		if (sli_cpc_frame_list_get_len(&data->rx_free) >=
		    CONFIG_MFD_SILABS_NCP_RX_QUEUE_SIZE) {
			irq_unlock(key);
			break;
		}
		irq_unlock(key);

		frame = sli_cpc_frame_new(&data->common.cpc_bus, true);
		if (!frame) {
			break;
		}
		key = irq_lock();
		sli_cpc_frame_list_push_back(&data->rx_free, frame);
		irq_unlock(key);
	}

	if (was_empty) {
		ncp_uart_wakeup(data);
	}
}

/*
 * CPC driver operations
 */

static sl_status_t ncp_uart_drv_init(sl_cpc_bus_t *bus)
{
	ARG_UNUSED(bus);

	return SL_STATUS_OK;
}

static sl_status_t ncp_uart_drv_start_rx(sl_cpc_bus_t *bus)
{
	struct ncp_uart_data *data = ncp_uart_data_from_bus(bus);
	const struct device *dev = data->dev;

	ncp_uart_refill_rx(dev);

	if (!data->rx_enabled && ncp_uart_rx_enable(dev) < 0) {
		return SL_STATUS_FAIL;
	}
	data->running = true;
	/* The co-processor may already have something to say */
	ncp_uart_wakeup(data);

	return SL_STATUS_OK;
}

static void ncp_uart_drv_deinit(sl_cpc_bus_t *bus)
{
	struct ncp_uart_data *data = ncp_uart_data_from_bus(bus);
	const struct ncp_uart_config *cfg = data->dev->config;

	data->running = false;
	uart_rx_disable(cfg->uart);
	k_work_cancel_delayable_sync(&data->work, &(struct k_work_sync){});
	ncp_uart_rx_reset(data);
	data->tx_frame = NULL;
}

static sl_status_t ncp_uart_drv_read(sl_cpc_bus_t *bus, sli_cpc_frame_list_t *frames)
{
	struct ncp_uart_data *data = ncp_uart_data_from_bus(bus);
	unsigned int key;

	key = irq_lock();
	sli_cpc_frame_list_extend(frames, &data->rx_pending);
	irq_unlock(key);

	return sli_cpc_frame_list_empty(frames) ? SL_STATUS_EMPTY : SL_STATUS_OK;
}

static uint32_t ncp_uart_drv_get_available_write_frame_slots(sl_cpc_bus_t *bus)
{
	struct ncp_uart_data *data = ncp_uart_data_from_bus(bus);
	uint32_t used;
	unsigned int key;

	key = irq_lock();
	used = sli_cpc_frame_list_get_len(&data->tx_queue);
	irq_unlock(key);

	if (used >= CONFIG_MFD_SILABS_NCP_TX_QUEUE_SIZE) {
		return 0;
	}

	return CONFIG_MFD_SILABS_NCP_TX_QUEUE_SIZE - used;
}

static uint32_t ncp_uart_drv_write(sl_cpc_bus_t *bus, sli_cpc_frame_list_t *frames)
{
	struct ncp_uart_data *data = ncp_uart_data_from_bus(bus);
	uint32_t count = sli_cpc_frame_list_get_len(frames);
	unsigned int key;

	if (ncp_uart_drv_get_available_write_frame_slots(bus) < count) {
		LOG_ERR("%s: TX queue overflow", data->dev->name);
		return 0;
	}

	key = irq_lock();
	sli_cpc_frame_list_extend(&data->tx_queue, frames);
	irq_unlock(key);

	LOG_DBG("%s: %u frame(s) from the core", data->dev->name, count);
	ncp_uart_wakeup(data);

	return count;
}

static void ncp_uart_drv_on_rx_frame_free(sl_cpc_bus_t *bus)
{
	ncp_uart_refill_rx(ncp_uart_data_from_bus(bus)->dev);
}

static const sli_cpc_drv_ops_t ncp_uart_drv_ops = {
	.init = ncp_uart_drv_init,
	.start_rx = ncp_uart_drv_start_rx,
	.deinit = ncp_uart_drv_deinit,
	.read = ncp_uart_drv_read,
	.write = ncp_uart_drv_write,
	.get_available_write_frame_slots = ncp_uart_drv_get_available_write_frame_slots,
	.on_rx_frame_free = ncp_uart_drv_on_rx_frame_free,
};

static int ncp_uart_init(const struct device *dev)
{
	const struct ncp_uart_config *cfg = dev->config;
	struct ncp_uart_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->uart)) {
		LOG_ERR("%s: UART not ready", dev->name);
		return -ENODEV;
	}

	data->dev = dev;
	sli_cpc_frame_list_init(&data->tx_queue);
	sli_cpc_frame_list_init(&data->tx_done);
	sli_cpc_frame_list_init(&data->rx_free);
	sli_cpc_frame_list_init(&data->rx_pending);
	ring_buf_init(&data->rx_rb, sizeof(data->rx_rb_buf), data->rx_rb_buf);
	k_work_init_delayable(&data->work, ncp_uart_work_handler);

	ret = uart_callback_set(cfg->uart, ncp_uart_callback, (void *)dev);
	if (ret < 0) {
		LOG_ERR("%s: asynchronous UART API not available: %d", dev->name, ret);
		return ret;
	}

	if (cfg->reset.port) {
		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	if (cfg->wake.port) {
		/* Keep the co-processor awake for now */
		ret = gpio_pin_configure_dt(&cfg->wake, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	/* Listen before the reset so the first frame of the co-processor is not
	 * lost. It stays in the ring buffer until the bus starts.
	 */
	ret = ncp_uart_rx_enable(dev);
	if (ret < 0) {
		return ret;
	}

	if (cfg->reset.port) {
		gpio_pin_set_dt(&cfg->reset, 1);
		k_sleep(K_MSEC(10));
		gpio_pin_set_dt(&cfg->reset, 0);
		k_sleep(K_MSEC(100));
	}

	return mfd_silabs_ncp_bus_start(dev, &ncp_uart_drv_ops);
}

/* The buffers handed to the UART must be usable by its DMA: nocache on the
 * targets with a data cache.
 */
#define NCP_UART_DEFINE(inst)                                                                      \
	static uint8_t ncp_uart_tx_buf_##inst[NCP_UART_MAX_FRAME] __nocache __aligned(4);         \
	static uint8_t ncp_uart_rx_bufs_##inst[2][CONFIG_MFD_SILABS_NCP_UART_RX_BUF_SIZE]          \
		__nocache __aligned(4);                                                            \
	static struct ncp_uart_data ncp_uart_data_##inst;                                          \
	static const struct ncp_uart_config ncp_uart_config_##inst = {                             \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.wake = GPIO_DT_SPEC_INST_GET_OR(inst, wake_gpios, {0}),                           \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
		.tx_buf = ncp_uart_tx_buf_##inst,                                                  \
		.rx_bufs = {ncp_uart_rx_bufs_##inst[0], ncp_uart_rx_bufs_##inst[1]},               \
		.rx_buf_size = CONFIG_MFD_SILABS_NCP_UART_RX_BUF_SIZE,                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ncp_uart_init, NULL, &ncp_uart_data_##inst,                   \
			      &ncp_uart_config_##inst, POST_KERNEL,                             \
			      CONFIG_MFD_SILABS_NCP_INIT_PRIORITY, NULL);

#define NCP_UART_INST(inst) COND_CODE_1(DT_INST_ON_BUS(inst, uart), (NCP_UART_DEFINE(inst)), ())

DT_INST_FOREACH_STATUS_OKAY(NCP_UART_INST)
