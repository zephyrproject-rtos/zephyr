/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Data and helpers shared by the transports of the "silabs,ncp" device.
 */

#ifndef ZEPHYR_DRIVERS_MFD_MFD_SILABS_NCP_H_
#define ZEPHYR_DRIVERS_MFD_MFD_SILABS_NCP_H_

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mfd/silabs_ncp.h>
#include <zephyr/kernel.h>
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
 * Common part of the data of a transport. Must be the first member of the
 * data of every transport: the services of the device find it there.
 */
struct ncp_common {
	/* CPC bus of the co-processor */
	struct sl_cpc_bus cpc_bus;
	/* Work queue of the transport, see mfd_silabs_ncp_wq_submit() */
	struct k_work_q wq;
	/* Stack of the work queue */
	K_KERNEL_STACK_MEMBER(wq_stack, CONFIG_MFD_SILABS_NCP_WQ_STACK_SIZE);
	const struct device *dev;
	/* Runs the transport on the work queue */
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
 * Initialize the common data of a "silabs,ncp" device and start its work queue,
 * with the given work handler. Called by the transports at the beginning of
 * their device initialization, before anything can schedule work.
 */
int ncp_common_init(struct ncp_common *common, const struct device *dev,
		    k_work_handler_t handler);

/*
 * Initialize and start the CPC bus with the given driver operations. Called by
 * the transports at the end of their device initialization.
 */
int ncp_bus_start(struct ncp_common *common, const sli_cpc_drv_ops_t *ops);

/*
 * Take every RX frame the pool can give, so that the transport receives ahead
 * of the core, and run the transport if it had none. Called when the bus
 * starts and whenever the core frees a RX frame.
 */
void ncp_refill_rx(struct ncp_common *common);

/*
 * Frames the core may hand to the transport. The transports send one frame per
 * transfer and keep one ready while the previous one is on the wire; a deeper
 * queue would only let the acknowledgment and the receive window written in
 * the header by the core go stale. ncp_drv_write_slots() is the same as a
 * driver operation.
 */
uint32_t ncp_write_slots(struct ncp_common *common);
uint32_t ncp_drv_write_slots(sl_cpc_bus_t *bus);

/*
 * Drop a RX frame after sli_cpc_alloc_rx_payload() was attempted on it: give
 * the payload chain back to the endpoint, release the endpoint reference and
 * free the frame. The core then calls on_rx_frame_free().
 */
void ncp_drop_rx_frame(sl_cpc_frame_t *frame);

/* Copy a contiguous buffer into a payload chain */
static inline void ncp_copy_to_chain(sl_cpc_buf_t *payload, const uint8_t *src, uint16_t len)
{
	sl_cpc_buf_t *buf = payload;

	while (buf != NULL && len > 0U) {
		uint16_t chunk = MIN(buf->len, len);

		(void)memcpy(buf->ptr, src, chunk);
		src += chunk;
		len -= chunk;
		buf = (sl_cpc_buf_t *)buf->node.node;
	}
}

/* Copy a payload chain into a contiguous buffer */
static inline void ncp_copy_from_chain(uint8_t *dst, const sl_cpc_buf_t *payload, uint16_t len)
{
	const sl_cpc_buf_t *buf = payload;

	while (buf != NULL && len > 0U) {
		uint16_t chunk = MIN(buf->len, len);

		(void)memcpy(dst, buf->ptr, chunk);
		dst += chunk;
		len -= chunk;
		buf = (const sl_cpc_buf_t *)buf->node.node;
	}
}

#endif /* ZEPHYR_DRIVERS_MFD_MFD_SILABS_NCP_H_ */
