/*
 * Copyright (c) 2026 Zephyr Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * VirtIO Input Device Driver (virtio-input, device ID 18)
 *
 * Specification: Virtual I/O Device (VIRTIO) Version 1.3, §5.8
 *
 * This driver sits on top of Zephyr's official VIRTIO transport layer
 * (drivers/virtio/virtio_pci.c, merged June 2025).  It does NOT perform
 * any raw PCI I/O or virtqueue ring manipulation itself – all of that is
 * handled by the transport driver via the virtio_* / virtq_* API defined in:
 *
 *   include/zephyr/drivers/virtio.h
 *   include/zephyr/drivers/virtio/virtqueue.h
 *
 * Driver layering:
 *
 *   Application
 *       └─ Zephyr input subsystem  (input_report_key / _rel / _abs / _sync)
 *           └─ virtio_input.c      (this file – device-class driver)
 *               └─ virtio_pci.c    (transport driver – virtio,pci compatible)
 *                   └─ PCI bus
 *
 * Initialisation sequence (VIRTIO spec §3.1.1):
 *   1. Transport driver resets device and sets ACKNOWLEDGE + DRIVER bits.
 *   2. This driver negotiates feature bits (none required for virtio-input).
 *   3. This driver calls virtio_init_virtqueues() for eventq + statusq.
 *   4. This driver pre-populates eventq with write-only receive buffers.
 *   5. virtio_finalize_init() sets DRIVER_OK – device starts sending events.
 *
 * Runtime event path:
 *   ISR (virtio_pci) --> virtq_receive_callback --> input_report_*
 */

#define DT_DRV_COMPAT virtio_input

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <string.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(virtio_input, CONFIG_INPUT_VIRTIO_LOG_LEVEL);

/* --------------------------------------------------------------------------
 * VirtIO Input wire structures  (spec 5.8.6)
 * --------------------------------------------------------------------------
 */

/** A single input event as transferred over the eventq */
struct virtio_input_event {
	uint16_t type;
	uint16_t code;
	int32_t  value;
} __packed;

/* Linux input event types mirrored to avoid kernel header dependency */
#define EV_SYN    0x00U
#define EV_KEY    0x01U
#define EV_REL    0x02U
#define EV_ABS    0x03U

#define BTN_LEFT  0x110U
#define BTN_TOUCH 0x14aU

#define SYN_REPORT 0x00U

/* --------------------------------------------------------------------------
 * VirtIO Input device-specific configuration space selectors (spec 5.8.4)
 * --------------------------------------------------------------------------
 */
#define VIRTIO_INPUT_CFG_UNSET     0x00U
#define VIRTIO_INPUT_CFG_ID_NAME   0x01U
#define VIRTIO_INPUT_CFG_ID_SERIAL 0x02U
#define VIRTIO_INPUT_CFG_ID_DEVIDS 0x03U
#define VIRTIO_INPUT_CFG_PROP_BITS 0x10U
#define VIRTIO_INPUT_CFG_EV_BITS   0x11U
#define VIRTIO_INPUT_CFG_ABS_INFO  0x12U

/**
 * @brief VirtIO input device-specific configuration space (spec 5.8.4)
 *
 * Obtained via virtio_get_device_specific_config().
 * The guest writes (select, subsel) then reads back size + union payload.
 */
struct virtio_input_config {
	uint8_t  select;
	uint8_t  subsel;
	uint8_t  size;
	uint8_t  reserved[5];
	union {
		char    string[128];
		uint8_t bitmap[128];
	} u;
} __packed;

/* Virtqueue indices for virtio-input */
#define VIRTIO_INPUT_EVENTQ     0U
#define VIRTIO_INPUT_STATUSQ    1U
#define VIRTIO_INPUT_NUM_QUEUES 2U

/* --------------------------------------------------------------------------
 * Per-buffer callback context
 *
 * virtq_add_buffer_chain() requires a callback + opaque pointer per chain.
 * We embed the event buffer itself here so a single allocation covers both.
 * --------------------------------------------------------------------------
 */
struct virtio_input_buf_ctx {
	/** The event buffer the device writes into */
	struct virtio_input_event evt;
	/** Back-pointer to the input device (for input_report_*) */
	const struct device *dev;
};

/* --------------------------------------------------------------------------
 * Driver private data (one instance per DT node)
 * --------------------------------------------------------------------------
 */
struct virtio_input_data {
	/** Per-slot contexts for eventq */
	struct virtio_input_buf_ctx buf_ctxs[CONFIG_INPUT_VIRTIO_QUEUE_SIZE];

	/** Actual eventq depth negotiated with the device */
	uint16_t eventq_size;
};

/* --------------------------------------------------------------------------
 * Driver configuration (static, from DT)
 * --------------------------------------------------------------------------
 */
struct virtio_input_config_dt {
	/** virtio transport device – parent phandle resolved at build time */
	const struct device *virtio_dev;
};

#define MY_SCREEN_WIDTH 1024 /* TODO: get from dts */
#define MY_SCREEN_HEIGHT 768 /* TODO: get from dts */

/* --------------------------------------------------------------------------
 * Event dispatch helper
 * --------------------------------------------------------------------------
 */
static void dispatch_event(const struct device *dev,
			    const struct virtio_input_event *evt)
{
	switch (evt->type) {
	case EV_KEY: {
		uint16_t report_code = evt->code;

		/* Map mouse buttons to input touch event */
		if (evt->code == BTN_LEFT || evt->code == BTN_TOUCH) {
			report_code = INPUT_BTN_TOUCH;
		}
		LOG_DBG("EV_KEY code=0x%04x report_code=0x%04x val=%d",
			 evt->code, report_code, evt->value);
		input_report_key(dev, report_code, evt->value != 0, true, K_NO_WAIT);
		break;
	}

	case EV_REL:
		LOG_DBG("EV_REL code=0x%04x val=%d", evt->code, evt->value);
		input_report_rel(dev, evt->code, evt->value, true, K_NO_WAIT);
		break;

	case EV_ABS:
		/* TODO: improve mapping */
		int32_t mapped_val = evt->value;

		if (evt->code == INPUT_ABS_X) {
			mapped_val = (evt->value * MY_SCREEN_WIDTH) / 32767;
		} else if (evt->code == INPUT_ABS_Y) {
			mapped_val = (evt->value * MY_SCREEN_HEIGHT) / 32767;
		}
		LOG_DBG("EV_ABS code=0x%04x val=%d", evt->code, mapped_val);
		input_report_abs(dev, evt->code, mapped_val, true, K_NO_WAIT);
		break;

	default:
		LOG_DBG("Unhandled event type=0x%04x code=0x%04x val=%d",
			evt->type, evt->code, evt->value);
		break;
	}
}

/* --------------------------------------------------------------------------
 * virtq_receive_callback
 *
 * Invoked by the transport driver (virtio_pci) when the device returns a
 * descriptor chain on the eventq used ring.  At this point @p opaque points
 * to the virtio_input_buf_ctx whose evt field has been filled by the device.
 *
 * After dispatching the event we immediately re-submit the same buffer so the
 * device can fill it again.  K_NO_WAIT is safe here because we are called
 * from the transport ISR or its deferred work.
 * --------------------------------------------------------------------------
 */
static void virtio_input_recv_cb(void *opaque, uint32_t used_len)
{
	struct virtio_input_buf_ctx *ctx = opaque;
	const struct virtio_input_config_dt *cfg = ctx->dev->config;
	int ret;

	LOG_DBG("%s: used_len=%u type=0x%04x code=0x%04x value=%d",
		__func__, used_len, ctx->evt.type, ctx->evt.code, ctx->evt.value);

	if (used_len < sizeof(struct virtio_input_event)) {
		LOG_ERR("Short virtio-input event (%u B), discarding", used_len);
		goto resubmit;
	}

	dispatch_event(ctx->dev, &ctx->evt);

resubmit:
	/* Recycle: re-add this buffer to the eventq available ring */
	struct virtq *eventq = virtio_get_virtqueue(cfg->virtio_dev,
						    VIRTIO_INPUT_EVENTQ);
	if (!eventq) {
		LOG_ERR("Cannot recycle eventq buffer: queue gone");
		return;
	}

	struct virtq_buf buf = {
		.addr = &ctx->evt,
		.len  = sizeof(struct virtio_input_event),
	};

	/*
	 * device_readable_count = 0 => all bufs in the chain are
	 * device-writeable (VIRTQ_DESC_F_WRITE is set by the virtqueue layer).
	 */
	ret = virtq_add_buffer_chain(eventq, &buf, 1U, 0U,
				     virtio_input_recv_cb, ctx, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("Failed to recycle eventq buffer: %d", ret);
		return;
	}

	virtio_notify_virtqueue(cfg->virtio_dev, VIRTIO_INPUT_EVENTQ);
}

/* --------------------------------------------------------------------------
 * virtio_enumerate_queues callback
 *
 * Called once per queue by virtio_init_virtqueues().  Returns the desired
 * queue depth, which must be <= max_queue_size.
 * --------------------------------------------------------------------------
 */
static uint16_t virtio_input_enumerate_queues(uint16_t queue_idx,
					       uint16_t max_queue_size,
					       void *opaque)
{
	struct virtio_input_data *data = opaque;
	uint16_t desired;

	LOG_DBG("eventq: queue_idx=%u", queue_idx);

	switch (queue_idx) {
	case VIRTIO_INPUT_EVENTQ:
		desired = MIN(max_queue_size,
			      (uint16_t)CONFIG_INPUT_VIRTIO_QUEUE_SIZE);
		data->eventq_size = desired;
		LOG_DBG("eventq: requested size=%u (device max=%u)",
			desired, max_queue_size);
		return desired;

	case VIRTIO_INPUT_STATUSQ:
		/*
		 * statusq carries LED state / force-feedback from driver to
		 * device.  We do not implement that; return 1 (minimum) so the
		 * device knows the queue exists but is not actively used.
		 */
		LOG_DBG("statusq: size=1 (unused)");
		return 1U;

	default:
		LOG_ERR("Unexpected virtqueue index %u requested", queue_idx);
		return max_queue_size;
	}
}

/* --------------------------------------------------------------------------
 * Driver initialisation
 * --------------------------------------------------------------------------
 */
static int virtio_input_init(const struct device *dev)
{
	const struct virtio_input_config_dt *cfg = dev->config;
	struct virtio_input_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->virtio_dev)) {
		LOG_ERR("VIRTIO transport device %s is not ready",
			cfg->virtio_dev->name);
		return -ENODEV;
	}

	/* ------------------------------------------------------------------
	 * Step 1: Feature bit negotiation
	 *
	 * virtio-input defines no mandatory device-type feature bits (spec
	 * 5.8.3).  We do not request any optional bits either, so we call
	 * virtio_commit_feature_bits() immediately to write FEATURES_OK.
	 *
	 * VIRTIO_F_VERSION_1 is handled internally by the transport driver.
	 * ------------------------------------------------------------------
	 */
	ret = virtio_commit_feature_bits(cfg->virtio_dev);
	if (ret != 0) {
		LOG_ERR("Feature negotiation failed: %d", ret);
		return ret;
	}
	LOG_DBG("Step 1: virtio_commit_feature_bits() completed successfully");

	/* ------------------------------------------------------------------
	 * Step 2: Virtqueue initialisation
	 *
	 * The enumerate callback fills data->eventq_size as a side-effect so
	 * we know how many buffer contexts to allocate in step 3.
	 * ------------------------------------------------------------------
	 */
	ret = virtio_init_virtqueues(cfg->virtio_dev,
				     VIRTIO_INPUT_NUM_QUEUES,
				     virtio_input_enumerate_queues,
				     data);
	if (ret != 0) {
		LOG_ERR("virtio_init_virtqueues failed: %d", ret);
		return ret;
	}
	LOG_DBG("Step 2: virtio_init_virtqueues() completed, eventq_size=%u", data->eventq_size);

	struct virtq *eventq = virtio_get_virtqueue(cfg->virtio_dev,
						    VIRTIO_INPUT_EVENTQ);
	if (!eventq) {
		LOG_ERR("Failed to retrieve eventq handle");
		return -ENODEV;
	}

	for (uint16_t i = 0; i < data->eventq_size; i++) {
		data->buf_ctxs[i].dev = dev;

		struct virtq_buf buf = {
			.addr = &data->buf_ctxs[i].evt,
			.len  = sizeof(struct virtio_input_event),
		};

		ret = virtq_add_buffer_chain(eventq, &buf, 1U, 0U,
					     virtio_input_recv_cb,
					     &data->buf_ctxs[i],
					     K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("Failed to add eventq buf %u: %d", i, ret);
			return ret;
		}
	}

	/* Notify the device once after all buffers are added */
	virtio_notify_virtqueue(cfg->virtio_dev, VIRTIO_INPUT_EVENTQ);
	LOG_DBG("Step 3: Finished pre-populating eventq");

	/* ------------------------------------------------------------------
	 * Step 4: Finalize – writes DRIVER_OK, device starts sending events.
	 *
	 * Barrier ensures all descriptor writes from virtq_add_buffer_chain
	 * are visible to the device before DRIVER_OK is set, matching the
	 * pattern used in eth_virtio_net.c.
	 * ------------------------------------------------------------------
	 */
	barrier_dmem_fence_full();
	virtio_finalize_init(cfg->virtio_dev);
	LOG_DBG("Step 4: DRIVER_OK has been set, device should start sending events");

	LOG_DBG("virtio-input ready (transport: %s, eventq depth: %u)",
		cfg->virtio_dev->name, data->eventq_size);
	return 0;
}

/* --------------------------------------------------------------------------
 * Device tree instantiation macro
 *
 * The DT node for virtio_input is a child of the virtio,pci node.
 * DEVICE_DT_GET(DT_INST_PARENT(inst)) gives us the transport device handle.
 * --------------------------------------------------------------------------
 */
#define VIRTIO_INPUT_DEFINE(inst)                                  \
	static struct virtio_input_data virtio_input_data_##inst;  \
	static const struct virtio_input_config_dt                 \
		virtio_input_cfg_##inst = {                        \
		.virtio_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)), \
	};                                                         \
	DEVICE_DT_INST_DEFINE(inst,                                \
			      virtio_input_init,                   \
			      NULL,                                \
			      &virtio_input_data_##inst,           \
			      &virtio_input_cfg_##inst,            \
			      POST_KERNEL,                         \
			      2,                                   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(VIRTIO_INPUT_DEFINE)
