/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Services of the "silabs,ncp" device and helpers shared by its transports.
 */

#include <zephyr/logging/log.h>

#include "mfd_silabs_ncp.h"

LOG_MODULE_REGISTER(mfd_silabs_ncp, CONFIG_MFD_LOG_LEVEL);

struct sl_cpc_bus *mfd_silabs_ncp_cpc_bus(const struct device *dev)
{
	struct ncp_common *common = dev->data;

	return &common->cpc_bus;
}

int mfd_silabs_ncp_wq_submit(const struct device *dev, struct k_work *work)
{
	struct ncp_common *common = dev->data;

	return k_work_submit_to_queue(&common->wq, work);
}

int mfd_silabs_ncp_wq_schedule(const struct device *dev, struct k_work_delayable *dwork,
			       k_timeout_t delay)
{
	struct ncp_common *common = dev->data;

	return k_work_schedule_for_queue(&common->wq, dwork, delay);
}

int mfd_silabs_ncp_wq_reschedule(const struct device *dev, struct k_work_delayable *dwork,
				 k_timeout_t delay)
{
	struct ncp_common *common = dev->data;

	return k_work_reschedule_for_queue(&common->wq, dwork, delay);
}

int ncp_common_init(struct ncp_common *common, const struct device *dev,
		    k_work_handler_t handler)
{
	const struct k_work_queue_config cfg = {
		.name = dev->name,
	};

	common->dev = dev;
	sli_cpc_frame_list_init(&common->tx_queue);
	sli_cpc_frame_list_init(&common->tx_done);
	sli_cpc_frame_list_init(&common->rx_free);
	sli_cpc_frame_list_init(&common->rx_pending);
	k_work_init_delayable(&common->work, handler);
	k_work_queue_init(&common->wq);
	k_work_queue_start(&common->wq, common->wq_stack, K_KERNEL_STACK_SIZEOF(common->wq_stack),
			   K_PRIO_COOP(CONFIG_MFD_SILABS_NCP_WQ_PRIORITY), &cfg);

	return 0;
}

int ncp_bus_start(struct ncp_common *common, const sli_cpc_drv_ops_t *ops)
{
	const struct device *dev = common->dev;
	sl_cpc_bus_t *bus = &common->cpc_bus;
	const sl_cpc_bus_config_t cfg = {
		.is_secondary = false,
		.rx_frame_pool_count = CONFIG_MFD_SILABS_NCP_RX_FRAME_POOL_COUNT,
		.tx_frame_pool_count = CONFIG_MFD_SILABS_NCP_TX_FRAME_POOL_COUNT,
	};
	sl_status_t status;

	status = sli_cpc_bus_init(bus, &cfg, ops);
	if (status != SL_STATUS_OK) {
		LOG_ERR("%s: CPC bus init failed: %d", dev->name, status);
		return -EIO;
	}

	status = sl_cpc_bus_start(bus);
	if (status != SL_STATUS_OK) {
		LOG_ERR("%s: CPC bus start failed: %d", dev->name, status);
		sli_cpc_bus_deinit(bus);
		return -EIO;
	}

	return 0;
}

void ncp_refill_rx(struct ncp_common *common)
{
	sl_cpc_frame_t *frame;
	unsigned int key;
	bool was_empty;

	key = irq_lock();
	was_empty = sli_cpc_frame_list_empty(&common->rx_free);
	irq_unlock(key);

	frame = sli_cpc_frame_new(&common->cpc_bus, true);
	while (frame != NULL) {
		key = irq_lock();
		sli_cpc_frame_list_push_back(&common->rx_free, frame);
		irq_unlock(key);
		frame = sli_cpc_frame_new(&common->cpc_bus, true);
	}

	if (was_empty) {
		(void)k_work_reschedule_for_queue(&common->wq, &common->work, K_NO_WAIT);
	}
}

uint32_t ncp_write_slots(struct ncp_common *common)
{
	unsigned int key;
	bool empty;

	key = irq_lock();
	empty = sli_cpc_frame_list_empty(&common->tx_queue);
	irq_unlock(key);

	return empty ? 1U : 0U;
}

uint32_t ncp_drv_write_slots(sl_cpc_bus_t *bus)
{
	return ncp_write_slots(CONTAINER_OF(bus, struct ncp_common, cpc_bus));
}

void ncp_drop_rx_frame(sl_cpc_frame_t *frame)
{
	/* The payload comes from the endpoint, it cannot exist without it */
	__ASSERT(frame->ep != NULL || frame->payload == NULL, "payload without endpoint");

	if (frame->payload != NULL) {
		sli_cpc_buffer_chain_release(&frame->ep->rx_buffer_queue, frame->payload,
					     frame->ep->mtu);
		frame->payload = NULL;
	}
	sli_cpc_frame_put_ref(&frame);
}
