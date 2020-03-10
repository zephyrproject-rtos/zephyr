/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/i2c_async.h>
#include <sys/queued_seq.h>
#include <drivers/i2c.h>
#include <string.h>

static struct i2c_async *seq_mgr_to_i2c_async(struct sys_seq_mgr *mgr)
{
	struct queued_seq_mgr *mgrs =
			CONTAINER_OF(mgr, struct queued_seq_mgr, seq_mgr);
	return CONTAINER_OF(mgrs, struct i2c_async, mgrs);
}

static void i2c_callback(struct device *dev,
			 struct sys_notify *notify,
			 int res)
{
	struct i2c_async *i2c_async =
		CONTAINER_OF(notify, struct i2c_async, action_notify);

	sys_seq_finalize(&i2c_async->mgrs.seq_mgr, res, 0);
}

int i2c_async_sys_seq_xfer(struct sys_seq_mgr *mgr, void *data)
{
	struct i2c_async *i2c_async = seq_mgr_to_i2c_async(mgr);
	struct i2c_msg *msg = data;

	sys_notify_init_callback(&i2c_async->action_notify, i2c_callback);
	return i2c_single_transfer(i2c_async->dev,
				   msg,
				   i2c_async->addr,
				   &i2c_async->action_notify);
}

int i2c_async_sys_seq_address_set(struct sys_seq_mgr *mgr, void *data)
{
	uint16_t *new_addr = data;
	struct i2c_async *i2c_async = seq_mgr_to_i2c_async(mgr);

	i2c_async->addr = *new_addr;

	sys_seq_finalize(mgr, 0, 0);

	return 0;
}

int i2c_async_init(struct i2c_async *i2c_async, struct device *dev)
{
	i2c_async->dev = dev;
	i2c_async->addr = I2C_INVALID_ADDRESS;

	return queued_seq_init(&i2c_async->mgrs, NULL, &i2c_async->delay_timer);
}

int z_i2c_async_sync_transfer(struct device *dev, struct i2c_msg *msgs,
			      uint8_t num_msgs, uint16_t addr)
{
	struct queued_seq qop;
	int err;

	for (uint8_t i = 0; i < num_msgs; i++) {
		SYS_SEQ_DEFINE(, seq,
			I2C_SEQ_ACTION_ADDRESS_SET(, addr),
			I2C_SEQ_ACTION_MSG(, msgs[i].buf,
					 msgs[i].len, msgs[i].flags)
		);
		qop.seq = &seq;

		err = queued_operation_sync_submit(
					i2c_get_queued_operation_manager(dev),
					&qop.qop,
					QUEUED_OPERATION_PRIORITY_APPEND);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}
