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
