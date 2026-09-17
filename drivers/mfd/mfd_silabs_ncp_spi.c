/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Silicon Labs network co-processor over SPI: CPC primary transport.
 *
 * Wire protocol (see sl_cpc_drv_spi_peripheral.c on the secondary side):
 *
 *  1. Header phase. One chip-select cycle of 10 bytes, full duplex: the
 *     8-byte CPC header followed by its 16-bit CRC (little endian). A side
 *     that has nothing to send clocks zeros.
 *  2. The secondary processes the header and asserts the IRQ line when it is
 *     ready for the payload phase.
 *  3. Payload phase. One chip-select cycle, mandatory even when both payloads
 *     are empty: the secondary only hands the header to its core and
 *     completes its own transmission at the end of this cycle. Each side
 *     sends its payload followed by its 16-bit CRC, the cycle length is the
 *     largest of the two.
 *
 * While idle, the secondary asserts the IRQ line when it has a frame to send.
 *
 * The transport is a state machine run from the system work queue. It is
 * woken up by the SPI completion callback, the IRQ line, the CPC core (frames
 * to send) and the RX frame pool (frames released).
 */

#define DT_DRV_COMPAT silabs_ncp

#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mfd_silabs_ncp_spi, CONFIG_MFD_LOG_LEVEL);

#include "mfd_silabs_ncp.h"
#include "sli_cpc_crc.h"


#define NCP_SPI_CSUM_SIZE   2
#define NCP_SPI_HDR_SIZE    (SLI_CPC_HEADER_SIZE + NCP_SPI_CSUM_SIZE)
/* Number of sl_cpc_buf_t in a payload chain we can scatter/gather */
#define NCP_SPI_MAX_BUFS    8
#define NCP_SPI_IRQ_TIMEOUT_MS 1000
#define NCP_SPI_IRQ_TIMEOUT K_MSEC(NCP_SPI_IRQ_TIMEOUT_MS)
#define NCP_SPI_RETRY_DELAY K_MSEC(10)

/* Must match the capability blob of the SPI peripheral driver */
struct sli_cpc_drv_caps {
	uint8_t max_speed_le[4];
} __packed;

enum ncp_spi_state {
	NCP_SPI_IDLE,
	/* Header phase in progress */
	NCP_SPI_HEADER,
	/* Header exchanged, waiting for the co-processor to assert IRQ */
	NCP_SPI_WAIT_IRQ,
	/* Payload phase in progress */
	NCP_SPI_PAYLOAD,
};

struct ncp_spi_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec irq;
	struct gpio_dt_spec wake;
	struct gpio_dt_spec reset;
};

struct ncp_spi_data {
	struct mfd_silabs_ncp_data common;
	const struct device *dev;
	struct sli_cpc_drv_caps local_caps;
	/* SPI configuration in use. Drivers identify a configuration by its
	 * address, so clamping the frequency means switching to another copy.
	 */
	const struct spi_config *spi_cfg;
	struct spi_config spi_cfg_clamped;
	struct gpio_callback irq_cb;
	struct k_work_delayable work;
	enum ncp_spi_state state;
	/* Set by the SPI completion callback */
	atomic_t xfer_done;
	int xfer_result;
	/* Start of the wait for the co-processor, for the watchdog */
	int64_t irq_wait_start;
	bool running;
	/* Frames submitted by the core, waiting for transmission */
	sli_cpc_frame_list_t tx_queue;
	/* Frames sent on the wire, waiting to be handed back to the core */
	sli_cpc_frame_list_t tx_done;
	/* Pre-allocated RX frames */
	sli_cpc_frame_list_t rx_free;
	/* Received frames, waiting to be read by the core */
	sli_cpc_frame_list_t rx_pending;
	uint32_t tx_inflight;
	/* Current transaction */
	sl_cpc_frame_t *tx;
	sl_cpc_frame_t *rx;
	uint16_t tx_len;
	uint16_t rx_len;
	bool rx_valid;
	bool rx_dropped;
	uint16_t rx_csum_le;
	uint8_t tx_hdr[NCP_SPI_HDR_SIZE];
	uint8_t rx_hdr[NCP_SPI_HDR_SIZE];
	struct spi_buf tx_bufs[NCP_SPI_MAX_BUFS + 2];
	struct spi_buf rx_bufs[NCP_SPI_MAX_BUFS + 2];
	struct spi_buf_set tx_set;
	struct spi_buf_set rx_set;
};

static inline struct ncp_spi_data *ncp_spi_data_from_bus(sl_cpc_bus_t *bus)
{
	return CONTAINER_OF(bus, struct ncp_spi_data, common.cpc_bus);
}

static void ncp_spi_wakeup(struct ncp_spi_data *data)
{
	k_work_reschedule(&data->work, K_NO_WAIT);
}

static void ncp_spi_irq_handler(const struct device *port, struct gpio_callback *cb,
				gpio_port_pins_t pins)
{
	struct ncp_spi_data *data = CONTAINER_OF(cb, struct ncp_spi_data, irq_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	ncp_spi_wakeup(data);
}

static void ncp_spi_xfer_cb(const struct device *spi, int result, void *user_data)
{
	const struct device *dev = user_data;
	struct ncp_spi_data *data = dev->data;

	ARG_UNUSED(spi);

	data->xfer_result = result;
	atomic_set(&data->xfer_done, 1);
	ncp_spi_wakeup(data);
}

static int ncp_spi_xfer_start(const struct device *dev)
{
	const struct ncp_spi_config *cfg = dev->config;
	struct ncp_spi_data *data = dev->data;

	atomic_set(&data->xfer_done, 0);

	return spi_transceive_cb(cfg->spi.bus, data->spi_cfg, &data->tx_set, &data->rx_set,
				 ncp_spi_xfer_cb, (void *)dev);
}

static bool ncp_spi_all_zeros(const uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (buf[i]) {
			return false;
		}
	}
	return true;
}

/* Fill spi_buf entries from a payload chain. Returns the number of entries or
 * a negative value if the chain is too long.
 */
static int ncp_spi_chain_to_bufs(sl_cpc_buf_t *payload, uint16_t len, struct spi_buf *bufs,
				 size_t max_bufs)
{
	sl_cpc_buf_t *buf = payload;
	int count = 0;

	while (buf && len > 0) {
		uint16_t chunk = MIN(buf->len, len);

		if (count >= max_bufs) {
			return -ENOMEM;
		}
		bufs[count].buf = buf->ptr;
		bufs[count].len = chunk;
		count++;
		len -= chunk;
		buf = (sl_cpc_buf_t *)buf->node.node;
	}

	if (len > 0) {
		return -EINVAL;
	}

	return count;
}

static bool ncp_spi_transaction_needed(const struct device *dev)
{
	const struct ncp_spi_config *cfg = dev->config;
	struct ncp_spi_data *data = dev->data;
	bool needed;
	unsigned int key;

	if (!data->running) {
		return false;
	}

	key = irq_lock();
	needed = !sli_cpc_frame_list_empty(&data->rx_free) &&
		 (!sli_cpc_frame_list_empty(&data->tx_queue) || gpio_pin_get_dt(&cfg->irq) > 0);
	irq_unlock(key);

	return needed;
}

/* Return the frames of the current transaction to their queues */
static void ncp_spi_abort(const struct device *dev)
{
	struct ncp_spi_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	if (data->tx) {
		data->tx_inflight--;
		sli_cpc_frame_list_push_front(&data->tx_queue, data->tx);
	}
	if (data->rx) {
		sli_cpc_frame_list_push_back(&data->rx_free, data->rx);
	}
	irq_unlock(key);

	data->tx = NULL;
	data->rx = NULL;
	data->state = NCP_SPI_IDLE;
	k_work_reschedule(&data->work, NCP_SPI_RETRY_DELAY);
}

/* IDLE -> HEADER */
static void ncp_spi_start_header(const struct device *dev)
{
	struct ncp_spi_data *data = dev->data;
	uint16_t crc;
	unsigned int key;
	int ret;

	key = irq_lock();
	data->tx = sli_cpc_frame_list_pop(&data->tx_queue);
	data->rx = sli_cpc_frame_list_pop(&data->rx_free);
	if (data->tx) {
		data->tx_inflight++;
	}
	irq_unlock(key);

	data->tx_len = 0;
	data->rx_len = 0;
	data->rx_valid = false;
	data->rx_dropped = false;

	if (data->tx) {
		data->tx_len = sli_cpc_header_get_payload_size(sli_cpc_frame_get_header(data->tx));
		memcpy(data->tx_hdr, sli_cpc_frame_get_header(data->tx), SLI_CPC_HEADER_SIZE);
		crc = sli_cpc_get_crc_sw(data->tx_hdr, SLI_CPC_HEADER_SIZE);
		sys_put_le16(crc, &data->tx_hdr[SLI_CPC_HEADER_SIZE]);
	} else {
		memset(data->tx_hdr, 0, sizeof(data->tx_hdr));
	}

	data->tx_bufs[0].buf = data->tx_hdr;
	data->tx_bufs[0].len = sizeof(data->tx_hdr);
	data->tx_set.count = 1;
	data->rx_bufs[0].buf = data->rx_hdr;
	data->rx_bufs[0].len = sizeof(data->rx_hdr);
	data->rx_set.count = 1;

	data->state = NCP_SPI_HEADER;
	ret = ncp_spi_xfer_start(dev);
	if (ret < 0) {
		LOG_ERR("%s: header transfer failed: %d", dev->name, ret);
		ncp_spi_abort(dev);
	}
}

/* HEADER -> WAIT_IRQ: parse the received header */
static void ncp_spi_header_done(const struct device *dev)
{
	struct ncp_spi_data *data = dev->data;
	sl_cpc_frame_t *rx = data->rx;
	int status;
	uint16_t crc;

	data->state = NCP_SPI_WAIT_IRQ;
	data->irq_wait_start = k_uptime_get();

	if (ncp_spi_all_zeros(data->rx_hdr, sizeof(data->rx_hdr))) {
		return;
	}

	crc = sli_cpc_get_crc_sw(data->rx_hdr, SLI_CPC_HEADER_SIZE);
	if (crc != sys_get_le16(&data->rx_hdr[SLI_CPC_HEADER_SIZE])) {
		LOG_WRN("%s: invalid header checksum", dev->name);
		return;
	}

	memcpy(sli_cpc_frame_get_header(rx), data->rx_hdr, SLI_CPC_HEADER_SIZE);
	if (!sli_cpc_header_is_size_valid(sli_cpc_frame_get_header(rx))) {
		LOG_WRN("%s: invalid payload size", dev->name);
		return;
	}

	data->rx_valid = true;
	data->rx_len = sli_cpc_header_get_payload_size(sli_cpc_frame_get_header(rx));

	/* Bind the endpoint and allocate the payload chain */
	status = sli_cpc_alloc_rx_payload(&data->common.cpc_bus, rx);
	if (status) {
		LOG_WRN("%s: cannot allocate RX payload: %d", dev->name, status);
		data->rx_dropped = true;
	}
}

/* WAIT_IRQ -> PAYLOAD */
static void ncp_spi_start_payload(const struct device *dev)
{
	struct ncp_spi_data *data = dev->data;
	uint16_t tx_len = data->tx_len;
	uint16_t rx_len = data->rx_dropped ? 0 : data->rx_len;
	size_t tx_total = tx_len ? tx_len + NCP_SPI_CSUM_SIZE : 0;
	size_t rx_total = rx_len ? rx_len + NCP_SPI_CSUM_SIZE : 0;
	size_t xfer_len = MAX(tx_total, rx_total);
	int count;
	int ret;

	/* The payload phase is mandatory, clock a dummy byte if needed */
	if (!xfer_len) {
		xfer_len = 1;
	}

	count = 0;
	if (tx_len) {
		count = ncp_spi_chain_to_bufs(data->tx->payload, tx_len, data->tx_bufs,
					      NCP_SPI_MAX_BUFS);
		if (count < 0) {
			LOG_ERR("%s: TX payload chain too long", dev->name);
			ncp_spi_abort(dev);
			return;
		}
		data->tx_bufs[count].buf = &data->tx->payload_csum;
		data->tx_bufs[count].len = NCP_SPI_CSUM_SIZE;
		count++;
	}
	if (xfer_len > tx_total) {
		data->tx_bufs[count].buf = NULL;
		data->tx_bufs[count].len = xfer_len - tx_total;
		count++;
	}
	data->tx_set.count = count;

	count = 0;
	if (rx_len) {
		count = ncp_spi_chain_to_bufs(data->rx->payload, rx_len, data->rx_bufs,
					      NCP_SPI_MAX_BUFS);
		if (count < 0) {
			LOG_ERR("%s: RX payload chain too long", dev->name);
			ncp_spi_abort(dev);
			return;
		}
		data->rx_bufs[count].buf = &data->rx_csum_le;
		data->rx_bufs[count].len = NCP_SPI_CSUM_SIZE;
		count++;
	}
	if (xfer_len > rx_total) {
		data->rx_bufs[count].buf = NULL;
		data->rx_bufs[count].len = xfer_len - rx_total;
		count++;
	}
	data->rx_set.count = count;

	data->state = NCP_SPI_PAYLOAD;
	ret = ncp_spi_xfer_start(dev);
	if (ret < 0) {
		LOG_ERR("%s: payload transfer failed: %d", dev->name, ret);
		ncp_spi_abort(dev);
	}
}

/* PAYLOAD -> IDLE: hand the frames over to the core */
static void ncp_spi_payload_done(const struct device *dev)
{
	struct ncp_spi_data *data = dev->data;
	sl_cpc_bus_t *bus = &data->common.cpc_bus;
	sl_cpc_frame_t *tx = data->tx;
	sl_cpc_frame_t *rx = data->rx;
	unsigned int key;

	data->tx = NULL;
	data->rx = NULL;
	data->state = NCP_SPI_IDLE;

	if (data->rx_valid && !data->rx_dropped) {
		if (data->rx_len) {
			rx->payload_csum = sys_le16_to_cpu(data->rx_csum_le);
			rx->payload_csum_is_valid =
				sli_cpc_get_csum_payload(rx->payload) == rx->payload_csum;
		} else {
			rx->payload_csum_is_valid = true;
		}
		key = irq_lock();
		sli_cpc_frame_list_push_back(&data->rx_pending, rx);
		irq_unlock(key);
		sli_cpc_bus_notify_rx_data_from_drv(bus);
	} else if (data->rx_valid) {
		/* Frame dropped after the endpoint was bound: release it */
		mfd_silabs_ncp_drop_rx_frame(rx);
	} else {
		/* Nothing received: recycle the RX frame */
		key = irq_lock();
		sli_cpc_frame_list_push_back(&data->rx_free, rx);
		irq_unlock(key);
	}

	if (tx) {
		key = irq_lock();
		data->tx_inflight--;
		sli_cpc_frame_list_push_back(&data->tx_done, tx);
		irq_unlock(key);
		sli_cpc_bus_notify_tx_data_by_drv(bus, &data->tx_done);
	}
}

static void ncp_spi_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ncp_spi_data *data = CONTAINER_OF(dwork, struct ncp_spi_data, work);
	const struct device *dev = data->dev;
	const struct ncp_spi_config *cfg = dev->config;

	switch (data->state) {
	case NCP_SPI_IDLE:
		if (ncp_spi_transaction_needed(dev)) {
			ncp_spi_start_header(dev);
		}
		break;
	case NCP_SPI_HEADER:
		if (!atomic_get(&data->xfer_done)) {
			break;
		}
		if (data->xfer_result < 0) {
			LOG_ERR("%s: header transfer failed: %d", dev->name, data->xfer_result);
			ncp_spi_abort(dev);
			break;
		}
		ncp_spi_header_done(dev);
		__fallthrough;
	case NCP_SPI_WAIT_IRQ:
		if (gpio_pin_get_dt(&cfg->irq) <= 0) {
			/* The IRQ line wakes us up, this is only a watchdog */
			if (k_uptime_get() - data->irq_wait_start >= NCP_SPI_IRQ_TIMEOUT_MS) {
				LOG_WRN("%s: still waiting for the co-processor", dev->name);
				data->irq_wait_start = k_uptime_get();
			}
			k_work_reschedule(&data->work, NCP_SPI_IRQ_TIMEOUT);
			break;
		}
		ncp_spi_start_payload(dev);
		break;
	case NCP_SPI_PAYLOAD:
		if (!atomic_get(&data->xfer_done)) {
			break;
		}
		if (data->xfer_result < 0) {
			LOG_ERR("%s: payload transfer failed: %d", dev->name, data->xfer_result);
			data->rx_dropped = true;
		}
		ncp_spi_payload_done(dev);
		/* Look for the next transaction */
		ncp_spi_wakeup(data);
		break;
	}
}

/* Keep the pool of RX frames full. Called from the core when a RX frame is
 * released, and from the transport itself.
 */
static void ncp_spi_refill_rx(const struct device *dev)
{
	struct ncp_spi_data *data = dev->data;
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
		ncp_spi_wakeup(data);
	}
}

/*
 * CPC driver operations
 */

static sl_status_t ncp_spi_drv_init(sl_cpc_bus_t *bus)
{
	ARG_UNUSED(bus);

	return SL_STATUS_OK;
}

static sl_status_t ncp_spi_drv_start_rx(sl_cpc_bus_t *bus)
{
	struct ncp_spi_data *data = ncp_spi_data_from_bus(bus);
	const struct device *dev = data->dev;
	const struct ncp_spi_config *cfg = dev->config;
	int ret;

	ncp_spi_refill_rx(dev);

	ret = gpio_pin_interrupt_configure_dt(&cfg->irq, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: cannot enable IRQ: %d", dev->name, ret);
		return SL_STATUS_FAIL;
	}

	data->running = true;
	ncp_spi_wakeup(data);

	return SL_STATUS_OK;
}

static void ncp_spi_drv_deinit(sl_cpc_bus_t *bus)
{
	struct ncp_spi_data *data = ncp_spi_data_from_bus(bus);
	const struct device *dev = data->dev;
	const struct ncp_spi_config *cfg = dev->config;

	data->running = false;
	gpio_pin_interrupt_configure_dt(&cfg->irq, GPIO_INT_DISABLE);
}

static sl_status_t ncp_spi_drv_read(sl_cpc_bus_t *bus, sli_cpc_frame_list_t *frames)
{
	struct ncp_spi_data *data = ncp_spi_data_from_bus(bus);
	unsigned int key;

	key = irq_lock();
	sli_cpc_frame_list_extend(frames, &data->rx_pending);
	irq_unlock(key);

	return sli_cpc_frame_list_empty(frames) ? SL_STATUS_EMPTY : SL_STATUS_OK;
}

static uint32_t ncp_spi_drv_get_available_write_frame_slots(sl_cpc_bus_t *bus)
{
	struct ncp_spi_data *data = ncp_spi_data_from_bus(bus);
	uint32_t used;
	unsigned int key;

	key = irq_lock();
	used = sli_cpc_frame_list_get_len(&data->tx_queue) + data->tx_inflight;
	irq_unlock(key);

	if (used >= CONFIG_MFD_SILABS_NCP_TX_QUEUE_SIZE) {
		return 0;
	}

	return CONFIG_MFD_SILABS_NCP_TX_QUEUE_SIZE - used;
}

static uint32_t ncp_spi_drv_write(sl_cpc_bus_t *bus, sli_cpc_frame_list_t *frames)
{
	struct ncp_spi_data *data = ncp_spi_data_from_bus(bus);
	uint32_t count = sli_cpc_frame_list_get_len(frames);
	sl_cpc_frame_t *frame;
	unsigned int key;

	if (ncp_spi_drv_get_available_write_frame_slots(bus) < count) {
		LOG_ERR("%s: TX queue overflow", data->dev->name);
		return 0;
	}

	SLI_CPC_FRAME_LIST_FOR_EACH(frames, frame) {
		if (frame->payload && !frame->payload_csum_is_valid) {
			frame->payload_csum = sli_cpc_get_csum_payload(frame->payload);
			frame->payload_csum_is_valid = true;
		}
	}

	key = irq_lock();
	sli_cpc_frame_list_extend(&data->tx_queue, frames);
	irq_unlock(key);

	ncp_spi_wakeup(data);

	return count;
}

static void ncp_spi_drv_on_rx_frame_free(sl_cpc_bus_t *bus)
{
	ncp_spi_refill_rx(ncp_spi_data_from_bus(bus)->dev);
}

static void ncp_spi_drv_get_local_capabilities(sl_cpc_bus_t *bus, const void **caps_p,
					       uint16_t *caps_size_p)
{
	struct ncp_spi_data *data = ncp_spi_data_from_bus(bus);

	*caps_p = &data->local_caps;
	*caps_size_p = sizeof(data->local_caps);
}

static sl_status_t ncp_spi_drv_set_remote_capabilities(sl_cpc_bus_t *bus, const void *caps,
						       uint16_t caps_size)
{
	struct ncp_spi_data *data = ncp_spi_data_from_bus(bus);
	const struct sli_cpc_drv_caps *remote = caps;
	uint32_t remote_speed;

	if (caps_size != sizeof(*remote)) {
		return SL_STATUS_INVALID_PARAMETER;
	}

	remote_speed = sys_get_le32(remote->max_speed_le);
	if (remote_speed && remote_speed < data->spi_cfg->frequency) {
		LOG_INF("%s: clamping SPI frequency to %u Hz", data->dev->name, remote_speed);
		data->spi_cfg_clamped = *data->spi_cfg;
		data->spi_cfg_clamped.frequency = remote_speed;
		data->spi_cfg = &data->spi_cfg_clamped;
	}

	return SL_STATUS_OK;
}

static const sli_cpc_drv_ops_t ncp_spi_drv_ops = {
	.init = ncp_spi_drv_init,
	.start_rx = ncp_spi_drv_start_rx,
	.deinit = ncp_spi_drv_deinit,
	.read = ncp_spi_drv_read,
	.write = ncp_spi_drv_write,
	.get_available_write_frame_slots = ncp_spi_drv_get_available_write_frame_slots,
	.on_rx_frame_free = ncp_spi_drv_on_rx_frame_free,
	.get_local_capabilities = ncp_spi_drv_get_local_capabilities,
	.set_remote_capabilities = ncp_spi_drv_set_remote_capabilities,
};

static int ncp_spi_init(const struct device *dev)
{
	const struct ncp_spi_config *cfg = dev->config;
	struct ncp_spi_data *data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("%s: SPI bus not ready", dev->name);
		return -ENODEV;
	}

	data->dev = dev;
	data->spi_cfg = &cfg->spi.config;
	sys_put_le32(cfg->spi.config.frequency, data->local_caps.max_speed_le);

	sli_cpc_frame_list_init(&data->tx_queue);
	sli_cpc_frame_list_init(&data->tx_done);
	sli_cpc_frame_list_init(&data->rx_free);
	sli_cpc_frame_list_init(&data->rx_pending);
	data->tx_set.buffers = data->tx_bufs;
	data->rx_set.buffers = data->rx_bufs;
	data->state = NCP_SPI_IDLE;
	k_work_init_delayable(&data->work, ncp_spi_work_handler);

	if (cfg->reset.port) {
		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
		k_sleep(K_MSEC(10));
		gpio_pin_set_dt(&cfg->reset, 0);
		k_sleep(K_MSEC(100));
	}

	if (cfg->wake.port) {
		/* Keep the co-processor awake for now */
		ret = gpio_pin_configure_dt(&cfg->wake, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	ret = gpio_pin_configure_dt(&cfg->irq, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}
	gpio_init_callback(&data->irq_cb, ncp_spi_irq_handler, BIT(cfg->irq.pin));
	ret = gpio_add_callback(cfg->irq.port, &data->irq_cb);
	if (ret < 0) {
		return ret;
	}

	return mfd_silabs_ncp_bus_start(dev, &ncp_spi_drv_ops);
}

#define NCP_SPI_DEFINE(inst)                                                                       \
	static struct ncp_spi_data ncp_spi_data_##inst;                                            \
	static const struct ncp_spi_config ncp_spi_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) |     \
					    SPI_TRANSFER_MSB),                                 \
		.irq = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                                     \
		.wake = GPIO_DT_SPEC_INST_GET_OR(inst, wake_gpios, {0}),                           \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ncp_spi_init, NULL, &ncp_spi_data_##inst,                     \
			      &ncp_spi_config_##inst, POST_KERNEL,                             \
			      CONFIG_MFD_SILABS_NCP_INIT_PRIORITY, NULL);

#define NCP_SPI_INST(inst) COND_CODE_1(DT_INST_ON_BUS(inst, spi), (NCP_SPI_DEFINE(inst)), ())

DT_INST_FOREACH_STATUS_OKAY(NCP_SPI_INST)
