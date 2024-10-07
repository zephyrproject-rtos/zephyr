/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2016,2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/i3c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(i3c, CONFIG_I3C_LOG_LEVEL);

/* Statically allocated array of IBI work item nodes */
static struct i3c_ibi_work i3c_ibi_work_nodes[CONFIG_I3C_IBI_WORKQUEUE_LENGTH];

static K_KERNEL_STACK_DEFINE(i3c_ibi_work_q_stack,
			     CONFIG_I3C_IBI_WORKQUEUE_STACK_SIZE);

/* IBI workqueue */
static struct k_work_q i3c_ibi_work_q;

static sys_slist_t i3c_ibi_work_nodes_free;

static inline int ibi_work_submit(struct i3c_ibi_work *ibi_node)
{
	return k_work_submit_to_queue(&i3c_ibi_work_q, &ibi_node->work);
}

int i3c_ibi_work_enqueue(struct i3c_ibi_work *ibi_work)
{
	sys_snode_t *node;
	struct i3c_ibi_work *ibi_node;
	int ret;

	node = sys_slist_get(&i3c_ibi_work_nodes_free);
	if (node == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	ibi_node = (struct i3c_ibi_work *)node;

	(void)memcpy(ibi_node, ibi_work, sizeof(*ibi_node));

	ret = ibi_work_submit(ibi_node);
	if (ret >= 0) {
		ret = 0;
	}

out:
	return ret;
}

int i3c_ibi_work_enqueue_target_irq(struct i3c_device_desc *target,
				    uint8_t *payload, size_t payload_len)
{
	sys_snode_t *node;
	struct i3c_ibi_work *ibi_node;
	int ret;

	node = sys_slist_get(&i3c_ibi_work_nodes_free);
	if (node == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	ibi_node = (struct i3c_ibi_work *)node;

	ibi_node->type = I3C_IBI_TARGET_INTR;
	ibi_node->target = target;
	ibi_node->payload.payload_len = payload_len;

	if ((payload != NULL) && (payload_len > 0U)) {
		(void)memcpy(&ibi_node->payload.payload[0],
			     payload, payload_len);
	}

	ret = ibi_work_submit(ibi_node);
	if (ret >= 0) {
		ret = 0;
	}

out:
	return ret;
}

int i3c_ibi_work_enqueue_controller_request(struct i3c_device_desc *target)
{
	sys_snode_t *node;
	struct i3c_ibi_work *ibi_node;
	int ret;

	node = sys_slist_get(&i3c_ibi_work_nodes_free);
	if (node == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	ibi_node = (struct i3c_ibi_work *)node;

	ibi_node->type = I3C_IBI_CONTROLLER_ROLE_REQUEST;
	ibi_node->target = target;

	ret = ibi_work_submit(ibi_node);
	if (ret >= 0) {
		ret = 0;
	}

out:
	return ret;
}

int i3c_ibi_work_enqueue_hotjoin(const struct device *dev)
{
	sys_snode_t *node;
	struct i3c_ibi_work *ibi_node;
	int ret;

	node = sys_slist_get(&i3c_ibi_work_nodes_free);
	if (node == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	ibi_node = (struct i3c_ibi_work *)node;

	ibi_node->type = I3C_IBI_HOTJOIN;
	ibi_node->controller = dev;
	ibi_node->payload.payload_len = 0;

	ret = ibi_work_submit(ibi_node);
	if (ret >= 0) {
		ret = 0;
	}

out:
	return ret;
}

int i3c_ibi_work_enqueue_cb(const struct device *dev,
			    k_work_handler_t work_cb)
{
	sys_snode_t *node;
	struct i3c_ibi_work *ibi_node;
	int ret;

	node = sys_slist_get(&i3c_ibi_work_nodes_free);
	if (node == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	ibi_node = (struct i3c_ibi_work *)node;

	ibi_node->type = I3C_IBI_WORKQUEUE_CB;
	ibi_node->controller = dev;
	ibi_node->work_cb = work_cb;

	ret = ibi_work_submit(ibi_node);
	if (ret >= 0) {
		ret = 0;
	}

out:
	return ret;
}

static void i3c_ibi_work_handler(struct k_work *work)
{
	struct i3c_ibi_work *ibi_node = CONTAINER_OF(work, struct i3c_ibi_work, work);
	struct i3c_ibi_payload *payload;
	int ret = 0;

	if (IS_ENABLED(CONFIG_I3C_IBI_WORKQUEUE_VERBOSE_DEBUG) &&
	    ((uint32_t)ibi_node->type <= I3C_IBI_TYPE_MAX)) {
		LOG_DBG("Processing IBI work %p (type %d, len %u)",
			ibi_node, (int)ibi_node->type,
			ibi_node->payload.payload_len);

		if (ibi_node->payload.payload_len > 0U) {
			LOG_HEXDUMP_DBG(&ibi_node->payload.payload[0],
					ibi_node->payload.payload_len, "IBI Payload");
		}
	}

	switch (ibi_node->type) {
	case I3C_IBI_TARGET_INTR:
		if (ibi_node->payload.payload_len != 0U) {
			payload = &ibi_node->payload;
		} else {
			payload = NULL;
		}

		if (ibi_node->target->ibi_cb) {
			ret = ibi_node->target->ibi_cb(ibi_node->target, payload);
			if ((ret != 0) && (ret != -EBUSY)) {
				LOG_ERR("IBI work %p cb returns %d", ibi_node, ret);
			}
		} else {
			LOG_ERR("No IBI callback for target %s", ibi_node->target->dev->name);
		}
		break;

	case I3C_IBI_HOTJOIN:
		ret = i3c_do_daa(ibi_node->controller);
		if ((ret != 0) && (ret != -EBUSY)) {
			LOG_ERR("i3c_do_daa returns %d", ret);
		}

		if (i3c_bus_has_sec_controller(ibi_node->controller)) {
			ret = i3c_bus_deftgts(ibi_node->controller);
			if (ret != 0) {
				LOG_ERR("Error sending DEFTGTS");
			}
		}
		break;

	case I3C_IBI_WORKQUEUE_CB:
		if (ibi_node->work_cb != NULL) {
			ibi_node->work_cb(work);
		}
		break;

	case I3C_IBI_CONTROLLER_ROLE_REQUEST:
		ret = i3c_device_controller_handoff(ibi_node->target, true);
		if (ret != 0) {
			LOG_ERR("i3c_device_controller_handoff returns %d", ret);
		}
		break;

	default:
		/* Unknown IBI type: do nothing */
		LOG_DBG("Cannot process IBI type %d", (int)ibi_node->type);
		break;
	}

	if (ret == -EBUSY) {
		/* Retry if bus is busy. */
		if (ibi_work_submit(ibi_node) < 0) {
			LOG_ERR("Error re-adding IBI work %p", ibi_node);
		}
	} else {
		/* Add the now processed node back to the free list */
		sys_slist_append(&i3c_ibi_work_nodes_free, (sys_snode_t *)ibi_node);
	}
}

static int i3c_ibi_work_q_init(void)
{
	struct k_work_queue_config cfg = {
		.name = "i3c_ibi_workq",
		.no_yield = true,
	};

	/* Init the linked list of work item nodes */
	sys_slist_init(&i3c_ibi_work_nodes_free);

	for (int i = 0; i < ARRAY_SIZE(i3c_ibi_work_nodes); i++) {
		i3c_ibi_work_nodes[i].work.handler = i3c_ibi_work_handler;

		sys_slist_append(&i3c_ibi_work_nodes_free,
				 (sys_snode_t *)&i3c_ibi_work_nodes[i]);
	}

	/* Start the workqueue */
	k_work_queue_start(&i3c_ibi_work_q, i3c_ibi_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(i3c_ibi_work_q_stack),
			   CONFIG_I3C_IBI_WORKQUEUE_PRIORITY, &cfg);

	return 0;
}

SYS_INIT(i3c_ibi_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
