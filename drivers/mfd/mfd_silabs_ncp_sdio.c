/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Silicon Labs network co-processor over SDIO: CPC primary transport.
 *
 * Wire protocol (see sl_cpc_drv_sdio_device.c on the secondary side and
 * sl_cpc_drv_sdio_host.c for the reference primary):
 *
 *  - Frames are exchanged with CMD53 block transfers on function 1 at
 *    register 0 (fixed address). A transfer carries a 4-byte block header
 *    (frame count, currently always 1), then for each frame the 8-byte CPC
 *    header followed by the payload padded to 4 bytes. The whole transfer is
 *    padded to the block size. Payload integrity is covered by the SDIO CRC.
 *  - The secondary raises the card interrupt when it has a frame to send.
 *    The host acknowledges it through the function 1 INT_ID register, reads
 *    the advertised transfer length from the XFER_COUNT register and issues
 *    the CMD53 read.
 *
 * The transport runs from the system work queue: the SDIO API is blocking.
 */

#define DT_DRV_COMPAT silabs_ncp

#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sd/sdio.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mfd_silabs_ncp_sdio, CONFIG_MFD_LOG_LEVEL);

#include "mfd_silabs_ncp.h"


#define NCP_SDIO_BLOCK_SIZE     512
#define NCP_SDIO_ALIGN          4
#define NCP_SDIO_RETRY_DELAY    K_MSEC(10)
#define NCP_SDIO_POLL_INTERVAL  K_MSEC(CONFIG_MFD_SILABS_NCP_SDIO_POLL_INTERVAL_MS)

/* Function 1 registers */
#define NCP_SDIO_FN1_INT_ID     0x08 /* Interrupt identification */
#define NCP_SDIO_FN1_INT_EN     0x09 /* Interrupt enable */
#define NCP_SDIO_FN1_XFER_COUNT 0x0C /* Advertised CMD53 transfer length */
#define NCP_SDIO_INT_DATA_READY BIT(0)
/* Function 0 vendor register: auto clock wake on card interrupt */
#define NCP_SDIO_FN0_CLOCK_WAKE 0x18000

/* Largest transfer: block header + CPC header + payload, padded to the block */
#define NCP_SDIO_MAX_XFER                                                                          \
	ROUND_UP(sizeof(struct ncp_sdio_hdr_block) + SLI_CPC_HEADER_SIZE +                         \
			 ROUND_UP(SL_CPC_EP_MAX_PAYLOAD_SIZE, NCP_SDIO_ALIGN),                     \
		 NCP_SDIO_BLOCK_SIZE)

/* Must match the capability blob of the SDIO device driver */
struct sli_cpc_drv_caps {
	uint8_t max_aggregation;
} __packed;

struct ncp_sdio_hdr_block {
	uint8_t frame_count;
	uint8_t reserved[3];
};

struct ncp_sdio_config {
	const struct device *sdhc;
	struct gpio_dt_spec wake;
	struct gpio_dt_spec reset;
};

struct ncp_sdio_data {
	struct mfd_silabs_ncp_data common;
	const struct device *dev;
	struct sli_cpc_drv_caps local_caps;
	struct sli_cpc_drv_caps remote_caps;
	struct sd_card card;
	struct sdio_func func1;
	struct k_work_delayable work;
	atomic_t card_int_pending;
	/* The host controller has no card interrupt: poll the co-processor */
	bool polling;
	bool running;
	/* Frames submitted by the core, waiting for transmission */
	sli_cpc_frame_list_t tx_queue;
	/* Frames sent on the wire, waiting to be handed back to the core */
	sli_cpc_frame_list_t tx_done;
	/* Pre-allocated RX frames */
	sli_cpc_frame_list_t rx_free;
	/* Received frames, waiting to be read by the core */
	sli_cpc_frame_list_t rx_pending;
	uint8_t rx_buf[NCP_SDIO_MAX_XFER] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
	uint8_t tx_buf[NCP_SDIO_MAX_XFER] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
};

static inline struct ncp_sdio_data *ncp_sdio_data_from_bus(sl_cpc_bus_t *bus)
{
	return CONTAINER_OF(bus, struct ncp_sdio_data, common.cpc_bus);
}

static void ncp_sdio_wakeup(struct ncp_sdio_data *data)
{
	k_work_reschedule(&data->work, K_NO_WAIT);
}

static void ncp_sdio_card_int(const struct device *sdhc, int reason, const void *user_data)
{
	const struct device *dev = user_data;
	struct ncp_sdio_data *data = dev->data;

	ARG_UNUSED(sdhc);

	if (reason == SDHC_INT_SDIO) {
		atomic_set(&data->card_int_pending, 1);
		ncp_sdio_wakeup(data);
	}
}

static int ncp_sdio_enable_card_int(const struct device *dev)
{
	const struct ncp_sdio_config *cfg = dev->config;

	return sdhc_enable_interrupt(cfg->sdhc, ncp_sdio_card_int, SDHC_INT_SDIO, (void *)dev);
}

/* Parse the bounce buffer of a CMD53 read and hand the frame to the core */
static void ncp_sdio_rx_parse(const struct device *dev, size_t xfer_len)
{
	struct ncp_sdio_data *data = dev->data;
	const struct ncp_sdio_hdr_block *hdr_block = (const void *)data->rx_buf;
	sl_cpc_frame_t *frame;
	int status;
	size_t offset = sizeof(*hdr_block);
	uint16_t payload_len;
	unsigned int key;

	if (hdr_block->frame_count != 1) {
		LOG_ERR("%s: unsupported frame count %u", dev->name, hdr_block->frame_count);
		return;
	}

	key = irq_lock();
	frame = sli_cpc_frame_list_pop(&data->rx_free);
	irq_unlock(key);
	if (!frame) {
		LOG_WRN("%s: no RX frame, dropping", dev->name);
		return;
	}

	memcpy(sli_cpc_frame_get_header(frame), &data->rx_buf[offset], SLI_CPC_HEADER_SIZE);
	offset += SLI_CPC_HEADER_SIZE;

	if (!sli_cpc_header_is_size_valid(sli_cpc_frame_get_header(frame))) {
		LOG_ERR("%s: invalid payload size", dev->name);
		goto drop;
	}
	payload_len = sli_cpc_header_get_payload_size(sli_cpc_frame_get_header(frame));
	if (offset + ROUND_UP(payload_len, NCP_SDIO_ALIGN) > xfer_len) {
		LOG_ERR("%s: payload exceeds the transfer length", dev->name);
		goto drop;
	}

	/* Bind the endpoint and allocate the payload chain */
	status = sli_cpc_alloc_rx_payload(&data->common.cpc_bus, frame);
	if (status) {
		LOG_WRN("%s: cannot allocate RX payload: %d", dev->name, status);
		mfd_silabs_ncp_drop_rx_frame(frame);
		return;
	}
	if (payload_len > 0) {
		mfd_silabs_ncp_copy_to_chain(frame->payload, &data->rx_buf[offset], payload_len);
	}
	/* The SDIO CRC covers the wire */
	frame->payload_csum_is_valid = true;

	key = irq_lock();
	sli_cpc_frame_list_push_back(&data->rx_pending, frame);
	irq_unlock(key);
	sli_cpc_bus_notify_rx_data_from_drv(&data->common.cpc_bus);

	return;

drop:
	key = irq_lock();
	sli_cpc_frame_list_push_back(&data->rx_free, frame);
	irq_unlock(key);
}

/* Service a card interrupt: acknowledge it and read the pending transfer.
 * Returns true when the interrupt has been fully serviced.
 */
static bool ncp_sdio_rx(const struct device *dev)
{
	struct ncp_sdio_data *data = dev->data;
	uint8_t int_id = 0;
	uint8_t len_le[4];
	uint32_t xfer_len;
	bool no_free;
	unsigned int key;
	int ret;

	key = irq_lock();
	no_free = sli_cpc_frame_list_empty(&data->rx_free);
	irq_unlock(key);
	if (no_free) {
		/* Wait for the core to release a frame */
		return false;
	}

	ret = sdio_read_byte(&data->func1, NCP_SDIO_FN1_INT_ID, &int_id);
	if (ret < 0) {
		LOG_ERR("%s: INT_ID read failed: %d", dev->name, ret);
		return false;
	}
	if (!(int_id & NCP_SDIO_INT_DATA_READY)) {
		/* Nothing to read. XFER_COUNT keeps its previous value, so it must
		 * only be trusted after the co-processor signalled new data.
		 */
		return true;
	}
	ret = sdio_write_byte(&data->func1, NCP_SDIO_FN1_INT_ID, int_id);
	if (ret < 0) {
		LOG_ERR("%s: INT_ID write failed: %d", dev->name, ret);
		return false;
	}

	ret = sdio_read_addr(&data->func1, NCP_SDIO_FN1_XFER_COUNT, len_le, sizeof(len_le));
	if (ret < 0) {
		LOG_ERR("%s: XFER_COUNT read failed: %d", dev->name, ret);
		return false;
	}
	xfer_len = sys_get_le32(len_le);
	if (!xfer_len) {
		/* Spurious interrupt, nothing to read */
		return true;
	}
	if (xfer_len < sizeof(struct ncp_sdio_hdr_block) + SLI_CPC_HEADER_SIZE ||
	    xfer_len > sizeof(data->rx_buf) || xfer_len % NCP_SDIO_BLOCK_SIZE) {
		LOG_ERR("%s: invalid transfer length %u", dev->name, xfer_len);
		return true;
	}

	ret = sdio_read_blocks_fifo(&data->func1, 0, data->rx_buf, xfer_len / NCP_SDIO_BLOCK_SIZE);
	if (ret < 0) {
		LOG_ERR("%s: CMD53 read failed: %d", dev->name, ret);
		return false;
	}
	LOG_HEXDUMP_DBG(data->rx_buf, xfer_len, "rx");

	ncp_sdio_rx_parse(dev, xfer_len);

	return true;
}

/* Send one frame. Returns true when a frame was sent. */
static bool ncp_sdio_tx(const struct device *dev)
{
	struct ncp_sdio_data *data = dev->data;
	struct ncp_sdio_hdr_block *hdr_block = (void *)data->tx_buf;
	sl_cpc_frame_t *frame;
	size_t offset = sizeof(*hdr_block);
	size_t xfer_len;
	uint16_t payload_len;
	unsigned int key;
	int ret;

	key = irq_lock();
	frame = sli_cpc_frame_list_pop(&data->tx_queue);
	irq_unlock(key);
	if (!frame) {
		return false;
	}

	memset(hdr_block, 0, sizeof(*hdr_block));
	hdr_block->frame_count = 1;
	memcpy(&data->tx_buf[offset], sli_cpc_frame_get_header(frame), SLI_CPC_HEADER_SIZE);
	offset += SLI_CPC_HEADER_SIZE;

	payload_len = sli_cpc_header_get_payload_size(sli_cpc_frame_get_header(frame));
	if (payload_len > 0) {
		mfd_silabs_ncp_copy_from_chain(&data->tx_buf[offset], frame->payload, payload_len);
		offset += payload_len;
	}
	xfer_len = ROUND_UP(offset, NCP_SDIO_BLOCK_SIZE);
	memset(&data->tx_buf[offset], 0, xfer_len - offset);

	LOG_HEXDUMP_DBG(data->tx_buf, xfer_len, "tx");
	ret = sdio_write_blocks_fifo(&data->func1, 0, data->tx_buf, xfer_len / NCP_SDIO_BLOCK_SIZE);
	if (ret < 0) {
		LOG_ERR("%s: CMD53 write failed: %d", dev->name, ret);
		key = irq_lock();
		sli_cpc_frame_list_push_front(&data->tx_queue, frame);
		irq_unlock(key);
		k_work_reschedule(&data->work, NCP_SDIO_RETRY_DELAY);
		return false;
	}

	key = irq_lock();
	sli_cpc_frame_list_push_back(&data->tx_done, frame);
	irq_unlock(key);
	sli_cpc_bus_notify_tx_data_by_drv(&data->common.cpc_bus, &data->tx_done);

	return true;
}

static void ncp_sdio_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ncp_sdio_data *data = CONTAINER_OF(dwork, struct ncp_sdio_data, work);
	const struct device *dev = data->dev;

	if (!data->running) {
		return;
	}

	/* Service the card first, the secondary may be waiting for room */
	if (data->polling || atomic_get(&data->card_int_pending)) {
		if (ncp_sdio_rx(dev)) {
			atomic_set(&data->card_int_pending, 0);
			if (!data->polling) {
				/* Re-arm: a still asserted line raises a new event */
				ncp_sdio_enable_card_int(dev);
			}
		} else {
			k_work_reschedule(&data->work, NCP_SDIO_RETRY_DELAY);
		}
	}

	while (ncp_sdio_tx(dev)) {
	}

	if (data->polling) {
		k_work_schedule(&data->work, NCP_SDIO_POLL_INTERVAL);
	}
}

/* Keep the pool of RX frames full. Called from the core when a RX frame is
 * released, and from the transport itself.
 */
static void ncp_sdio_refill_rx(const struct device *dev)
{
	struct ncp_sdio_data *data = dev->data;
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
		ncp_sdio_wakeup(data);
	}
}

/*
 * CPC driver operations
 */

static int ncp_sdio_card_init(const struct device *dev)
{
	const struct ncp_sdio_config *cfg = dev->config;
	struct ncp_sdio_data *data = dev->data;
	uint8_t val;
	int ret;

	if (cfg->reset.port) {
		gpio_pin_set_dt(&cfg->reset, 1);
		k_sleep(K_MSEC(10));
		gpio_pin_set_dt(&cfg->reset, 0);
		k_sleep(K_MSEC(100));
	}

	ret = sd_init(cfg->sdhc, &data->card);
	if (ret < 0) {
		LOG_ERR("%s: card init failed: %d", dev->name, ret);
		return -EIO;
	}

	ret = sdio_init_func(&data->card, &data->func1, SDIO_FUNC_NUM_1);
	if (ret < 0) {
		LOG_ERR("%s: function init failed: %d", dev->name, ret);
		return -EIO;
	}

	ret = sdio_enable_func(&data->func1);
	if (ret < 0) {
		LOG_ERR("%s: function enable failed: %d", dev->name, ret);
		return -EIO;
	}

	ret = sdio_set_block_size(&data->func1, NCP_SDIO_BLOCK_SIZE);
	if (ret < 0) {
		LOG_ERR("%s: block size set failed: %d", dev->name, ret);
		return -EIO;
	}

	/* Master and function 1 interrupt enable (CCCR) */
	ret = sdio_read_byte(&data->card.func0, SDIO_CCCR_INT_EN, &val);
	if (!ret) {
		ret = sdio_write_byte(&data->card.func0, SDIO_CCCR_INT_EN, val | BIT(0) | BIT(1));
	}
	if (ret < 0) {
		LOG_ERR("%s: interrupt enable failed: %d", dev->name, ret);
		return -EIO;
	}

	/* Function 1: interrupt on data ready */
	ret = sdio_write_byte(&data->func1, NCP_SDIO_FN1_INT_EN, NCP_SDIO_INT_DATA_READY);
	if (ret < 0) {
		LOG_ERR("%s: function interrupt enable failed: %d", dev->name, ret);
		return -EIO;
	}

	/* Interrupt between blocks of a 4-bit transfer */
	ret = sdio_read_byte(&data->card.func0, SDIO_CCCR_CAPS, &val);
	if (!ret) {
		ret = sdio_write_byte(&data->card.func0, SDIO_CCCR_CAPS, val | SDIO_CCCR_CAPS_E4MI);
	}
	if (ret < 0) {
		LOG_ERR("%s: E4MI enable failed: %d", dev->name, ret);
		return -EIO;
	}

	/* Let the card wake its clock on interrupt */
	ret = sdio_write_byte(&data->card.func0, NCP_SDIO_FN0_CLOCK_WAKE, 0x01);
	if (ret < 0) {
		LOG_ERR("%s: clock wake enable failed: %d", dev->name, ret);
		return -EIO;
	}

	LOG_INF("%s: card ready", dev->name);

	return 0;
}

static sl_status_t ncp_sdio_drv_init(sl_cpc_bus_t *bus)
{
	struct ncp_sdio_data *data = ncp_sdio_data_from_bus(bus);

	data->local_caps.max_aggregation = 1;
	data->remote_caps.max_aggregation = 1;

	return SL_STATUS_OK;
}

static sl_status_t ncp_sdio_drv_start_rx(sl_cpc_bus_t *bus)
{
	struct ncp_sdio_data *data = ncp_sdio_data_from_bus(bus);
	const struct device *dev = data->dev;
	int ret;

	ncp_sdio_refill_rx(dev);

	data->running = true;
	ret = ncp_sdio_enable_card_int(dev);
	if (ret == -ENOSYS) {
		LOG_WRN("%s: no card interrupt, polling every %d ms", dev->name,
			CONFIG_MFD_SILABS_NCP_SDIO_POLL_INTERVAL_MS);
		data->polling = true;
	} else if (ret < 0) {
		LOG_ERR("%s: cannot enable card interrupt: %d", dev->name, ret);
		return SL_STATUS_FAIL;
	}

	/* The card may already have something to say */
	atomic_set(&data->card_int_pending, 1);
	ncp_sdio_wakeup(data);

	return SL_STATUS_OK;
}

static void ncp_sdio_drv_deinit(sl_cpc_bus_t *bus)
{
	struct ncp_sdio_data *data = ncp_sdio_data_from_bus(bus);
	const struct ncp_sdio_config *cfg = data->dev->config;

	data->running = false;
	if (!data->polling) {
		sdhc_disable_interrupt(cfg->sdhc, SDHC_INT_SDIO);
	}
}

static sl_status_t ncp_sdio_drv_read(sl_cpc_bus_t *bus, sli_cpc_frame_list_t *frames)
{
	struct ncp_sdio_data *data = ncp_sdio_data_from_bus(bus);
	unsigned int key;

	key = irq_lock();
	sli_cpc_frame_list_extend(frames, &data->rx_pending);
	irq_unlock(key);

	return sli_cpc_frame_list_empty(frames) ? SL_STATUS_EMPTY : SL_STATUS_OK;
}

static uint32_t ncp_sdio_drv_get_available_write_frame_slots(sl_cpc_bus_t *bus)
{
	struct ncp_sdio_data *data = ncp_sdio_data_from_bus(bus);
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

static uint32_t ncp_sdio_drv_write(sl_cpc_bus_t *bus, sli_cpc_frame_list_t *frames)
{
	struct ncp_sdio_data *data = ncp_sdio_data_from_bus(bus);
	uint32_t count = sli_cpc_frame_list_get_len(frames);
	unsigned int key;

	if (ncp_sdio_drv_get_available_write_frame_slots(bus) < count) {
		LOG_ERR("%s: TX queue overflow", data->dev->name);
		return 0;
	}

	key = irq_lock();
	sli_cpc_frame_list_extend(&data->tx_queue, frames);
	irq_unlock(key);

	ncp_sdio_wakeup(data);

	return count;
}

static void ncp_sdio_drv_on_rx_frame_free(sl_cpc_bus_t *bus)
{
	ncp_sdio_refill_rx(ncp_sdio_data_from_bus(bus)->dev);
}

static void ncp_sdio_drv_get_local_capabilities(sl_cpc_bus_t *bus, const void **caps_p,
						uint16_t *caps_size_p)
{
	struct ncp_sdio_data *data = ncp_sdio_data_from_bus(bus);

	*caps_p = &data->local_caps;
	*caps_size_p = sizeof(data->local_caps);
}

static sl_status_t ncp_sdio_drv_set_remote_capabilities(sl_cpc_bus_t *bus, const void *caps,
							uint16_t caps_size)
{
	struct ncp_sdio_data *data = ncp_sdio_data_from_bus(bus);

	if (caps_size != sizeof(data->remote_caps)) {
		return SL_STATUS_INVALID_PARAMETER;
	}

	memcpy(&data->remote_caps, caps, caps_size);

	return SL_STATUS_OK;
}

static const sli_cpc_drv_ops_t ncp_sdio_drv_ops = {
	.init = ncp_sdio_drv_init,
	.start_rx = ncp_sdio_drv_start_rx,
	.deinit = ncp_sdio_drv_deinit,
	.read = ncp_sdio_drv_read,
	.write = ncp_sdio_drv_write,
	.get_available_write_frame_slots = ncp_sdio_drv_get_available_write_frame_slots,
	.on_rx_frame_free = ncp_sdio_drv_on_rx_frame_free,
	.get_local_capabilities = ncp_sdio_drv_get_local_capabilities,
	.set_remote_capabilities = ncp_sdio_drv_set_remote_capabilities,
};

static int ncp_sdio_init(const struct device *dev)
{
	const struct ncp_sdio_config *cfg = dev->config;
	struct ncp_sdio_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->sdhc)) {
		LOG_ERR("%s: SDHC not ready", dev->name);
		return -ENODEV;
	}

	data->dev = dev;
	sli_cpc_frame_list_init(&data->tx_queue);
	sli_cpc_frame_list_init(&data->tx_done);
	sli_cpc_frame_list_init(&data->rx_free);
	sli_cpc_frame_list_init(&data->rx_pending);
	k_work_init_delayable(&data->work, ncp_sdio_work_handler);

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

	ret = ncp_sdio_card_init(dev);
	if (ret < 0) {
		return ret;
	}

	return mfd_silabs_ncp_bus_start(dev, &ncp_sdio_drv_ops);
}

#define NCP_SDIO_DEFINE(inst)                                                                      \
	static struct ncp_sdio_data ncp_sdio_data_##inst;                                          \
	static const struct ncp_sdio_config ncp_sdio_config_##inst = {                             \
		.sdhc = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                       \
		.wake = GPIO_DT_SPEC_INST_GET_OR(inst, wake_gpios, {0}),                           \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, ncp_sdio_init, NULL, &ncp_sdio_data_##inst,                   \
			      &ncp_sdio_config_##inst, POST_KERNEL,                             \
			      CONFIG_MFD_SILABS_NCP_INIT_PRIORITY, NULL);

#define NCP_SDIO_INST(inst) COND_CODE_1(DT_INST_ON_BUS(inst, sd), (NCP_SDIO_DEFINE(inst)), ())

DT_INST_FOREACH_STATUS_OKAY(NCP_SDIO_INST)
