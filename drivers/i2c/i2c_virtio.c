/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Driver for the VIRTIO I2C adapter device (virtio spec 1.3, section 5.16).
 *
 * The wire protocol puts one struct i2c_msg in one descriptor chain, so a
 * transfer of N messages is N chains. They are all queued before the device is
 * notified, which costs a multi-message transfer one round trip instead of one
 * per message, and every register read is two messages.
 *
 * Every operation is a round trip to the device, so none of the I2C API calls
 * may be issued from an ISR.
 */

#define DT_DRV_COMPAT virtio_i2c

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(i2c_virtio, CONFIG_I2C_LOG_LEVEL);

#define VIRTIO_I2C_REQUESTQ 0

/* Mandatory: zero-length requests, and M_RD carrying the direction. */
#define VIRTIO_I2C_F_ZERO_LENGTH_REQUEST 0

/* Fail the following request too, if this one fails. */
#define VIRTIO_I2C_FLAGS_FAIL_NEXT BIT(0)
/* This message is a read; otherwise it is a write. */
#define VIRTIO_I2C_FLAGS_M_RD      BIT(1)

#define VIRTIO_I2C_MSG_OK 0

struct virtio_i2c_out_hdr {
	uint16_t addr;
	uint16_t padding;
	uint32_t flags;
} __packed;

struct virtio_i2c_in_hdr {
	uint8_t status;
} __packed;

/* Per-message state, one slot per message of the batch in flight. */
struct i2c_virtio_slot {
	struct virtio_i2c_out_hdr out_hdr;
	struct virtio_i2c_in_hdr in_hdr;
	/* The completion callback is handed a slot and nothing else. */
	const struct device *dev;
};

struct i2c_virtio_config {
	const struct device *vdev;
};

struct i2c_virtio_data {
	/* serializes the batch in flight and the slots it uses */
	struct k_mutex lock;
	/* given once per completed chain, taken once per queued chain */
	struct k_sem done;
	struct virtq *requestq;
	/* outcome of the batch in flight, written by the completion callback */
	int status;
	struct i2c_virtio_slot slots[CONFIG_I2C_VIRTIO_MAX_MSGS];
};

/*
 * Runs on the virtio ISR. Chains may come back out of order -- legal, and
 * harmless here, because a message is judged as it returns and only a failure
 * writes to the shared status.
 */
static void i2c_virtio_chain_cb(void *opaque, uint32_t used_len)
{
	struct i2c_virtio_slot *slot = opaque;
	struct i2c_virtio_data *data = slot->dev->data;

	if (used_len < sizeof(slot->in_hdr)) {
		LOG_ERR("short response (%u bytes)", used_len);
		data->status = -EIO;
	} else if (slot->in_hdr.status != VIRTIO_I2C_MSG_OK) {
		LOG_DBG("message rejected by the device (status %u)", slot->in_hdr.status);
		data->status = -EIO;
	} else {
		/* Success: leave the shared status alone. */
	}

	k_sem_give(&data->done);
}

/* Queue one message as a chain, without notifying the device. */
static int i2c_virtio_queue_msg(struct i2c_virtio_data *data, struct i2c_virtio_slot *slot,
				struct i2c_msg *msg, uint16_t addr, bool last,
				virtq_receive_callback cb)
{
	/* Device-readable buffers must come first in the array. */
	struct virtq_buf bufs[3];
	uint16_t nbufs = 0;
	uint16_t readable;
	uint32_t flags = 0;

	if (i2c_is_read_op(msg)) {
		flags |= VIRTIO_I2C_FLAGS_M_RD;
	}
	/*
	 * FAIL_NEXT groups the messages the device runs back to back, with a
	 * restart rather than a stop between them, and is cleared on the last
	 * message of each group: a stop closes a group, and so does the end of
	 * the transfer.
	 */
	if (!last && !i2c_is_stop_op(msg)) {
		flags |= VIRTIO_I2C_FLAGS_FAIL_NEXT;
	}

	/* The address goes out shifted, leaving room for the R/W bit. */
	slot->out_hdr.addr = sys_cpu_to_le16(addr << 1);
	slot->out_hdr.padding = 0;
	slot->out_hdr.flags = sys_cpu_to_le32(flags);
	slot->in_hdr.status = 0;

	bufs[nbufs++] = (struct virtq_buf){.addr = &slot->out_hdr, .len = sizeof(slot->out_hdr)};

	if (msg->len > 0) {
		bufs[nbufs++] = (struct virtq_buf){.addr = msg->buf, .len = msg->len};
	}

	/* The payload is device-readable on a write, device-writable on a read. */
	readable = i2c_is_read_op(msg) ? 1 : nbufs;

	bufs[nbufs++] = (struct virtq_buf){.addr = &slot->in_hdr, .len = sizeof(slot->in_hdr)};

	return virtq_add_buffer_chain(data->requestq, bufs, nbufs, readable, cb, slot, K_NO_WAIT);
}

static int i2c_virtio_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	const struct i2c_virtio_config *cfg = dev->config;
	struct i2c_virtio_data *data = dev->data;
	int ret = 0;

	if (num_msgs == 0) {
		return 0;
	}

	for (uint8_t i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			LOG_ERR("10-bit addressing is not supported");
			return -ENOTSUP;
		}
		if (msgs[i].len > 0 && msgs[i].buf == NULL) {
			return -EINVAL;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (uint16_t base = 0; base < num_msgs;) {
		uint16_t batch = MIN(CONFIG_I2C_VIRTIO_MAX_MSGS, num_msgs - base);
		uint16_t queued = 0;

		/*
		 * A batch ends where its first group does, so a group that
		 * fails takes the rest of the transfer with it: nothing past
		 * the stop is ever queued.
		 */
		for (uint16_t i = 0; i < batch; i++) {
			if (i2c_is_stop_op(&msgs[base + i])) {
				batch = i + 1;
				break;
			}
		}

		/*
		 * Cleared before the first chain is queued: the device may
		 * complete one as soon as it is made available.
		 */
		data->status = 0;

		for (uint16_t i = 0; i < batch; i++) {
			/*
			 * "last" is per transfer, not per batch: a group
			 * longer than a batch has to keep FAIL_NEXT set
			 * across the seam.
			 */
			bool last = (base + i) == (num_msgs - 1);

			ret = i2c_virtio_queue_msg(data, &data->slots[i], &msgs[base + i], addr,
						   last, i2c_virtio_chain_cb);
			if (ret != 0) {
				LOG_ERR("failed to queue message %u: %d", base + i, ret);
				break;
			}
			queued++;
		}

		if (queued > 0) {
			virtio_notify_virtqueue(cfg->vdev, VIRTIO_I2C_REQUESTQ);
		}

		/*
		 * Wait for every chain that made it onto the queue, even when
		 * one failed to queue: the device owns those buffers until it
		 * returns them, and abandoning them would corrupt the slots the
		 * next transfer reuses.
		 */
		for (uint16_t i = 0; i < queued; i++) {
			k_sem_take(&data->done, K_FOREVER);
		}

		if (ret != 0) {
			goto out;
		}

		ret = data->status;
		if (ret != 0) {
			goto out;
		}

		base += batch;
	}

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int i2c_virtio_configure(const struct device *dev, uint32_t config)
{
	ARG_UNUSED(dev);

	if (config & I2C_ADDR_10_BITS) {
		return -ENOTSUP;
	}
	if (!(config & I2C_MODE_CONTROLLER)) {
		return -ENOTSUP;
	}
	/*
	 * The device has no bus-speed control -- there is no bus. Any speed is
	 * accepted and ignored, so that drivers written for real parts, which
	 * configure a speed as a matter of course, bind unmodified.
	 */
	return 0;
}

static int i2c_virtio_get_config(const struct device *dev, uint32_t *config)
{
	ARG_UNUSED(dev);

	*config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD);
	return 0;
}

static DEVICE_API(i2c, i2c_virtio_api) = {
	.configure = i2c_virtio_configure,
	.get_config = i2c_virtio_get_config,
	.transfer = i2c_virtio_transfer,
};

static uint16_t i2c_virtio_enum_queues_cb(uint16_t q_index, uint16_t q_size_max, void *unused)
{
	ARG_UNUSED(unused);

	if (q_index != VIRTIO_I2C_REQUESTQ) {
		return 0;
	}
	/* Up to three descriptors per message, for a whole batch at once. */
	return MIN(NHPOT(3 * CONFIG_I2C_VIRTIO_MAX_MSGS), q_size_max);
}

static int i2c_virtio_init(const struct device *dev)
{
	const struct i2c_virtio_config *cfg = dev->config;
	struct i2c_virtio_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->vdev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->vdev);
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	k_sem_init(&data->done, 0, CONFIG_I2C_VIRTIO_MAX_MSGS);
	for (uint16_t i = 0; i < CONFIG_I2C_VIRTIO_MAX_MSGS; i++) {
		data->slots[i].dev = dev;
	}

	if (!virtio_read_device_feature_bit(cfg->vdev, VIRTIO_I2C_F_ZERO_LENGTH_REQUEST)) {
		LOG_ERR("device does not offer VIRTIO_I2C_F_ZERO_LENGTH_REQUEST");
		return -ENOTSUP;
	}

	ret = virtio_write_driver_feature_bit(cfg->vdev, VIRTIO_I2C_F_ZERO_LENGTH_REQUEST, true);
	if (ret != 0) {
		LOG_ERR("failed to accept VIRTIO_I2C_F_ZERO_LENGTH_REQUEST: %d", ret);
		return ret;
	}

	ret = virtio_commit_feature_bits(cfg->vdev);
	if (ret != 0) {
		LOG_ERR("virtio_commit_feature_bits failed: %d", ret);
		return ret;
	}

	ret = virtio_init_virtqueues(cfg->vdev, 1, i2c_virtio_enum_queues_cb, NULL);
	if (ret != 0) {
		LOG_ERR("virtio_init_virtqueues failed: %d", ret);
		return ret;
	}

	data->requestq = virtio_get_virtqueue(cfg->vdev, VIRTIO_I2C_REQUESTQ);
	if (data->requestq == NULL) {
		LOG_ERR("failed to get the request virtqueue");
		return -ENODEV;
	}

	virtio_finalize_init(cfg->vdev);

	LOG_DBG("ready, up to %u messages per batch", CONFIG_I2C_VIRTIO_MAX_MSGS);

	return 0;
}

#define I2C_VIRTIO_DEFINE(inst)                                                                    \
	static struct i2c_virtio_data i2c_virtio_data_##inst;                                      \
	static const struct i2c_virtio_config i2c_virtio_config_##inst = {                         \
		.vdev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                       \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_virtio_init, NULL, &i2c_virtio_data_##inst,            \
				  &i2c_virtio_config_##inst, POST_KERNEL,                          \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_virtio_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_VIRTIO_DEFINE)
