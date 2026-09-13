/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/vhost.h>
#include <zephyr/drivers/vhost/vringh.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(vhost);

#define QUEUE_READY_HANDLER_NAME(node_name) UTIL_CAT(node_name, _queue_ready)

#define QUEUE_READY_HANDLER_PROTODECL(sym)                                                         \
	void QUEUE_READY_HANDLER_NAME(sym)(const struct device *dev, uint16_t qid, void *data);

#define DECL_HANDLER(inst, x, y)                                                                   \
	QUEUE_READY_HANDLER_PROTODECL(DT_NODE_FULL_NAME_UNQUOTED(DT_INST(inst, xen_vhost_mmio)))

#define REGISTER_HANDLER(inst, x, y)                                                               \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_GET(DT_INST(inst, xen_vhost_mmio));           \
		register_handler(dev, QUEUE_READY_HANDLER_NAME(DT_NODE_FULL_NAME_UNQUOTED(         \
					      DT_INST(inst, xen_vhost_mmio))));                    \
	}

DT_COMPAT_FOREACH_STATUS_OKAY_VARGS(xen_vhost_mmio, DECL_HANDLER, ())

int register_handler(const struct device *dev,
		     void (*handler)(const struct device *, uint16_t, void *))
{
	if (!device_is_ready(dev)) {
		LOG_ERR("VHost device %s not ready", dev->name);
		return -ENODEV;
	}

	LOG_INF("VHost device %s ready", dev->name);
	vhost_register_virtq_ready_cb(dev, handler, (void *)dev);

	return 0;
}

int main(void)
{
	DT_COMPAT_FOREACH_STATUS_OKAY_VARGS(xen_vhost_mmio, REGISTER_HANDLER, ())

	LOG_INF("VHost sample application started, waiting for guest connections...");
	k_sleep(K_FOREVER);

	return 0;
}
