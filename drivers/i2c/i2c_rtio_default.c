/*
 * Copyright (c) 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/rtio.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/slist.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(i2c_rtio, CONFIG_I2C_LOG_LEVEL);

struct i2c_rtio_fallback_work {
	sys_snode_t node;
	struct k_work work;
	struct rtio_iodev_sqe *iodev_sqe;
};

static struct k_spinlock rtio_work_pool_lock;
static sys_slist_t rtio_work_pool = SYS_SLIST_STATIC_INIT(&rtio_work_pool);
static struct i2c_rtio_fallback_work
	rtio_work_mem[CONFIG_I2C_RTIO_MAX_PENDING_SYS_WORKQUEUE_REQUESTS];

static int init_rtio_work_pool(void)
{
	for (int i = 0; i < ARRAY_SIZE(rtio_work_mem); ++i) {
		sys_slist_append(&rtio_work_pool, &rtio_work_mem[i].node);
	}
	return 0;
}

SYS_INIT(init_rtio_work_pool, PRE_KERNEL_1, 0);

void i2c_iodev_submit_work_handler(struct k_work *item)
{
	struct i2c_rtio_fallback_work *work =
		CONTAINER_OF(item, struct i2c_rtio_fallback_work, work);
	struct rtio_iodev_sqe *iodev_sqe = work->iodev_sqe;
	const struct i2c_dt_spec *dt_spec = (const struct i2c_dt_spec *)iodev_sqe->sqe.iodev->data;
	const struct device *dev = dt_spec->bus;

	LOG_DBG("Executing entry %p with SQE %p", (void *)work, (void *)iodev_sqe);

	/* Only support OP_RX and OP_TX for now */
	if (iodev_sqe->sqe.op != RTIO_OP_RX && iodev_sqe->sqe.op != RTIO_OP_TX) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		goto end;
	}

	/* Convert the iodev_sqe back to an i2c_msg */
	struct i2c_msg msg = {
		.buf = iodev_sqe->sqe.buf,
		.len = iodev_sqe->sqe.buf_len,
		.flags = ((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_STOP) ? I2C_MSG_STOP : 0) |
			 ((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_RESTART) ? I2C_MSG_RESTART
										: 0) |
			 ((iodev_sqe->sqe.iodev_flags & RTIO_IODEV_I2C_10_BITS)
				  ? I2C_MSG_ADDR_10_BITS
				  : 0) |
			 ((iodev_sqe->sqe.op == RTIO_OP_RX) ? I2C_MSG_READ : 0) |
			 ((iodev_sqe->sqe.op == RTIO_OP_TX) ? I2C_MSG_WRITE : 0),
	};

	int rc = i2c_transfer(dev, &msg, 1, dt_spec->addr);

	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}
end:
	K_SPINLOCK(&rtio_work_pool_lock) {
		sys_slist_append(&rtio_work_pool, &work->node);
	}
}

void i2c_iodev_submit_fallback(struct rtio_iodev_sqe *iodev_sqe)
{
	LOG_DBG("Executing fallback for SQE %p", (void *)iodev_sqe);

	sys_snode_t *node;

	K_SPINLOCK(&rtio_work_pool_lock) {
		node = sys_slist_get(&rtio_work_pool);
	}

	if (node == NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}
	struct i2c_rtio_fallback_work *entry =
		CONTAINER_OF(node, struct i2c_rtio_fallback_work, node);

	LOG_DBG("Scheduling entry %p", (void *)entry);

	entry->iodev_sqe = iodev_sqe;
	k_work_init(&entry->work, i2c_iodev_submit_work_handler);
	k_work_submit(&entry->work);
}
