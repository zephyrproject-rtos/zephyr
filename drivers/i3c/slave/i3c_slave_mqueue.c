/*
 * Copyright (c) 2023 Zephyr Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT i3c_slave_mqueue

#include <soc.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/sys/sys_io.h>

#include <stdlib.h>
#include "i3c/i3c_nct.h"

#define LOG_LEVEL CONFIG_I3C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_slave_mqueue);

struct i3c_slave_mqueue_config {
	char *controller_name;
	int msg_size;
	int num_of_msgs;
	int mdb;
};

struct i3c_slave_mqueue_obj {
	const struct device *i3c_controller;
	struct i3c_slave_payload *msg_curr;
	struct i3c_slave_payload *msg_queue;
	int in;
	int out;
};

#define DEV_CFG(dev)			((struct i3c_slave_mqueue_config *)(dev)->config)
#define DEV_DATA(dev)			((struct i3c_slave_mqueue_obj *)(dev)->data)

static struct i3c_slave_payload *i3c_slave_mqueue_write_requested(const struct device *dev)
{
	struct i3c_slave_mqueue_obj *obj = DEV_DATA(dev);

	return obj->msg_curr;
}

static void i3c_slave_mqueue_write_done(const struct device *dev)
{
	struct i3c_slave_mqueue_obj *obj = DEV_DATA(dev);
	struct i3c_slave_mqueue_config *config = DEV_CFG(dev);

#ifdef DBG_DUMP
	int i;
	uint8_t *buf = (uint8_t *)obj->msg_curr->buf;

	printk("%s: %d %d\n", __func__, obj->in, obj->out);
	for (i = 0; i < obj->msg_curr->size; i++) {
		printk("%02x\n", buf[i]);
	}
#endif
	/* update pointer */
	obj->in = (obj->in + 1) & (config->num_of_msgs - 1);
	obj->msg_curr = &obj->msg_queue[obj->in];

	/* if queue full, skip the oldest un-read message */
	if (obj->in == obj->out) {
		LOG_WRN("buffer overflow\n");
		obj->out = (obj->out + 1) & (config->num_of_msgs - 1);
	}
}

static const struct i3c_slave_callbacks i3c_slave_mqueue_callbacks = {
	.write_requested = i3c_slave_mqueue_write_requested,
	.write_done = i3c_slave_mqueue_write_done,
};

/**
 * @brief application reads the data from the message queue
 *
 * @param dev i3c slave mqueue device
 * @return int -1: message queue empty
 */
int i3c_slave_mqueue_read(const struct device *dev, uint8_t *dest, int budget)
{
	struct i3c_slave_mqueue_config *config = DEV_CFG(dev);
	struct i3c_slave_mqueue_obj *obj = DEV_DATA(dev);
	struct i3c_slave_payload *msg;
	int ret;

	if (obj->out == obj->in) {
		return 0;
	}

	msg = &obj->msg_queue[obj->out];
	ret = (msg->size > budget) ? budget : msg->size;
	memcpy(dest, msg->buf, ret);

	obj->out = (obj->out + 1) & (config->num_of_msgs - 1);

	return ret;
}

extern int target_wait_for_tx_fifo_empty(k_timeout_t timeout);

int i3c_slave_mqueue_write(const struct device *dev, uint8_t *src, int size)
{
	struct i3c_slave_mqueue_config *config = DEV_CFG(dev);
	struct i3c_slave_mqueue_obj *obj = DEV_DATA(dev);
	struct i3c_ibi_payload ibi;
	uint32_t event_en;
	int ret;
	uint8_t dynamic_addr;

	// the i3c_controller is the target device node we try to manupulate
	ret = i3c_slave_get_dynamic_addr(obj->i3c_controller, &dynamic_addr);
	if (ret) {
		return -ENOTCONN;
	}

	ret = i3c_slave_get_event_enabling(obj->i3c_controller, &event_en);
	if (ret || !(event_en & I3C_SLAVE_EVENT_SIR)) {
		return -EACCES;
	}

	struct i3c_slave_payload read_data;

	read_data.buf = src;
	read_data.size = size;

	if (IS_MDB_PENDING_READ_NOTIFY(config->mdb)) {
		/* response with ibi */
#if 0
		uint32_t ibi_data = config->mdb;

		ibi.buf = (uint8_t *)&ibi_data;
		ibi.size = 1;
#else
		ibi.payload[0] = (uint8_t)config->mdb;
		ibi.payload_len = 1;
#endif

		ret = i3c_slave_put_read_data(obj->i3c_controller, &read_data, &ibi);
		if (ret == 0) {
			target_wait_for_tx_fifo_empty(K_FOREVER);
		}

		return ret;
	}

	/* response without ibi, master should support retry */
	ret = i3c_slave_put_read_data(obj->i3c_controller, &read_data, NULL);
	if (ret == 0) {
		target_wait_for_tx_fifo_empty(K_FOREVER);
	}

	return ret;
}

static void i3c_slave_mqueue_init(const struct device *dev)
{
	struct i3c_slave_mqueue_config *config = DEV_CFG(dev);
	struct i3c_slave_mqueue_obj *obj = DEV_DATA(dev);
	struct i3c_slave_setup slave_data;
	uint8_t *buf;
	int i;

	LOG_DBG("msg size %d, n %d\n", config->msg_size, config->num_of_msgs);
	// LOG_DBG("bus name : %s\n", config->controller_name);
	LOG_DBG("bus name : %s\n", obj->i3c_controller->name);
	__ASSERT((config->num_of_msgs & (config->num_of_msgs - 1)) == 0,
		 "number of msgs must be power of 2\n");

	// obj->i3c_controller = device_get_binding(config->controller_name);

	buf = (uint8_t *)malloc(config->msg_size * config->num_of_msgs);
	if (!buf) {
		LOG_ERR("failed to create message buffer\n");
		return;
	}

	obj->msg_queue = (struct i3c_slave_payload *)malloc(sizeof(struct i3c_slave_payload) *
							      config->num_of_msgs);
	if (!obj->msg_queue) {
		LOG_ERR("failed to create message queue\n");
		return;
	}

	for (i = 0; i < config->num_of_msgs; i++) {
		obj->msg_queue[i].buf = buf + (i * config->msg_size);
		obj->msg_queue[i].size = 0;
	}

	obj->in = 0;
	obj->out = 0;
	obj->msg_curr = &obj->msg_queue[0];

	slave_data.max_payload_len = config->msg_size;
	slave_data.callbacks = &i3c_slave_mqueue_callbacks;
	slave_data.dev = dev;
	i3c_slave_register(obj->i3c_controller, &slave_data);
}

BUILD_ASSERT(CONFIG_I3C_SLAVE_INIT_PRIORITY > CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	     "I3C controller must be initialized prior to target device initialization");

#define I3C_SLAVE_MQUEUE_INIT(n)                               \
	static int i3c_slave_mqueue_config_func_##n(const struct device *dev);                     \
	static const struct i3c_slave_mqueue_config i3c_slave_mqueue_config_##n = {                \
		/*.controller_name = DT_INST_BUS_LABEL(n),*/                                           \
		.msg_size = DT_INST_PROP(n, msg_size),                                             \
		.num_of_msgs = DT_INST_PROP(n, num_of_msgs),                                       \
		.mdb = DT_INST_PROP(n, mandatory_data_byte),                                       \
	};                                                                                         \
												   \
	static struct i3c_slave_mqueue_obj i3c_slave_mqueue_obj##n = {							\
		.i3c_controller = DEVICE_DT_GET(DT_INST_BUS(n)),								  					\
	};                                														\
												   \
	DEVICE_DT_INST_DEFINE(n, &i3c_slave_mqueue_config_func_##n, NULL,                          \
			      &i3c_slave_mqueue_obj##n, &i3c_slave_mqueue_config_##n, POST_KERNEL, \
			      CONFIG_I3C_SLAVE_INIT_PRIORITY, NULL);                     \
												   \
	static int i3c_slave_mqueue_config_func_##n(const struct device *dev)                      \
	{                                                                                          \
		i3c_slave_mqueue_init(dev);                                                        \
		return 0;                                                                          \
	}

DT_INST_FOREACH_STATUS_OKAY(I3C_SLAVE_MQUEUE_INIT)
