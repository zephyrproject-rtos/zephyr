/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Helpers shared by the transports of the "silabs,ncp" device. The including
 * file must have registered its log module.
 */

#ifndef ZEPHYR_DRIVERS_MFD_MFD_SILABS_NCP_H_
#define ZEPHYR_DRIVERS_MFD_MFD_SILABS_NCP_H_

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/silabs_ncp.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "sl_cpc.h"
#include "sl_cpc_bus.h"
#include "sl_cpc_config.h"
#include "sli_cpc_bus.h"
#include "sli_cpc_drv.h"
#include "sli_cpc_frame.h"
#include "sli_cpc_frame_list.h"
#include "sli_cpc_hdr.h"
#include "sli_cpc_memory.h"

/*
 * Data shared by the transports. Must be the first member of the data of every
 * transport, so that the common data is at the start of the device data.
 */
struct mfd_silabs_ncp_transport {
	struct mfd_silabs_ncp_data common;
	const struct device *dev;
	/* Runs the transport on the work queue of the device */
	struct k_work_delayable work;
	/* The CPC bus is started */
	bool running;
	/* Frames submitted by the core, waiting for transmission */
	sli_cpc_frame_list_t tx_queue;
	/* Frames sent on the wire, waiting to be handed back to the core */
	sli_cpc_frame_list_t tx_done;
	/* Pre-allocated RX frames */
	sli_cpc_frame_list_t rx_free;
	/* Received frames, waiting to be read by the core */
	sli_cpc_frame_list_t rx_pending;
};

/*
 * Initialize the shared data of a "silabs,ncp" device and start its work
 * queue, with the given work handler. Called by the transports at the
 * beginning of their device initialization, before anything can schedule
 * work.
 */
static inline int mfd_silabs_ncp_common_init(const struct device *dev,
					     struct mfd_silabs_ncp_transport *xport,
					     k_work_handler_t handler)
{
	const struct k_work_queue_config cfg = {
		.name = dev->name,
	};

	xport->dev = dev;
	sli_cpc_frame_list_init(&xport->tx_queue);
	sli_cpc_frame_list_init(&xport->tx_done);
	sli_cpc_frame_list_init(&xport->rx_free);
	sli_cpc_frame_list_init(&xport->rx_pending);
	k_work_init_delayable(&xport->work, handler);
	k_work_queue_init(&xport->common.wq);
	k_work_queue_start(&xport->common.wq, xport->common.wq_stack,
			   K_KERNEL_STACK_SIZEOF(xport->common.wq_stack),
			   K_PRIO_COOP(CONFIG_MFD_SILABS_NCP_WQ_PRIORITY), &cfg);

	return 0;
}

/* Run the transport after the given delay, replacing a pending delay */
static inline int mfd_silabs_ncp_reschedule(struct mfd_silabs_ncp_transport *xport,
					    k_timeout_t delay)
{
	return mfd_silabs_ncp_wq_reschedule(xport->dev, &xport->work, delay);
}

/* Run the transport after the given delay, unless a run is already pending */
static inline int mfd_silabs_ncp_schedule(struct mfd_silabs_ncp_transport *xport,
					  k_timeout_t delay)
{
	return mfd_silabs_ncp_wq_schedule(xport->dev, &xport->work, delay);
}

/* Run the transport as soon as possible */
static inline void mfd_silabs_ncp_wakeup(struct mfd_silabs_ncp_transport *xport)
{
	mfd_silabs_ncp_reschedule(xport, K_NO_WAIT);
}

/*
 * Initialize and start the CPC bus of a "silabs,ncp" device with the given
 * driver operations. Called by the transports at the end of their device
 * initialization.
 */
static inline int mfd_silabs_ncp_bus_start(const struct device *dev, const sli_cpc_drv_ops_t *ops)
{
	sl_cpc_bus_t *bus = mfd_silabs_ncp_cpc_bus(dev);
	const sl_cpc_bus_config_t cfg = {
		.is_secondary = false,
		.rx_frame_pool_count = CONFIG_MFD_SILABS_NCP_RX_FRAME_POOL_COUNT,
		.tx_frame_pool_count = CONFIG_MFD_SILABS_NCP_TX_FRAME_POOL_COUNT,
	};
	int status;

	status = sli_cpc_bus_init(bus, &cfg, ops);
	if (status) {
		LOG_ERR("%s: CPC bus init failed: %d", dev->name, status);
		return -EIO;
	}

	status = sl_cpc_bus_start(bus);
	if (status) {
		LOG_ERR("%s: CPC bus start failed: %d", dev->name, status);
		sli_cpc_bus_deinit(bus);
		return -EIO;
	}

	return 0;
}

/*
 * Drop a RX frame after sli_cpc_alloc_rx_payload() was attempted on it: give
 * the payload chain back to the endpoint, release the endpoint reference and
 * free the frame. The core then calls on_rx_frame_free().
 */
static inline void mfd_silabs_ncp_drop_rx_frame(sl_cpc_frame_t *frame)
{
	if (frame->ep && frame->payload) {
		sli_cpc_buffer_chain_release(&frame->ep->rx_buffer_queue, frame->payload,
					     frame->ep->mtu);
		frame->payload = NULL;
	}
	sli_cpc_frame_put_ref(&frame);
}

/* Copy a contiguous buffer into a payload chain */
static inline void mfd_silabs_ncp_copy_to_chain(sl_cpc_buf_t *payload, const uint8_t *src,
						uint16_t len)
{
	sl_cpc_buf_t *buf = payload;

	while (buf && len > 0) {
		uint16_t chunk = MIN(buf->len, len);

		memcpy(buf->ptr, src, chunk);
		src += chunk;
		len -= chunk;
		buf = (sl_cpc_buf_t *)buf->node.node;
	}
}

/* Copy a payload chain into a contiguous buffer */
static inline void mfd_silabs_ncp_copy_from_chain(uint8_t *dst, const sl_cpc_buf_t *payload,
						  uint16_t len)
{
	const sl_cpc_buf_t *buf = payload;

	while (buf && len > 0) {
		uint16_t chunk = MIN(buf->len, len);

		memcpy(dst, buf->ptr, chunk);
		dst += chunk;
		len -= chunk;
		buf = (const sl_cpc_buf_t *)buf->node.node;
	}
}

#endif /* ZEPHYR_DRIVERS_MFD_MFD_SILABS_NCP_H_ */
