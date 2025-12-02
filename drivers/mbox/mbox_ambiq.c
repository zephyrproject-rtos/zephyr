/*
 * Copyright (c) 2025 Ambiq Micro, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_mbox_ipc

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/types.h>

#include <am_mcu_apollo.h>

#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbox_ambiq);

/* Supported maximum channels is 4. Two channels are used for signalling (e.g.,
 * IPC service), a specific 32-bit magic number will be sent to triggered an
 * interrupt to the peer. Another two channels are used for sending the 32-bit
 * message data.
 */
#define MBOX_MAX_CHANNELS  4
#define MBOX_MSG_SIZE      4 /* Mailbox message size: 4 bytes */
#define MBOX_INVALID_MAGIC 0xFFFFFFFF

struct mbox_ambiq_data_t {
	const struct device *dev;
	mbox_callback_t mbox_cb[MBOX_MAX_CHANNELS];
	void *user_data[MBOX_MAX_CHANNELS];
	uint32_t enabled_mask;
};

static struct mbox_ambiq_data_t ambiq_mbox_data;

static uint32_t mbox_signal_magic[MBOX_MAX_CHANNELS] = {AM_HAL_IPC_MBOX_SIGNAL_MSG_M2D,
							AM_HAL_IPC_MBOX_SIGNAL_MSG_D2M,
							MBOX_INVALID_MAGIC, MBOX_INVALID_MAGIC};

static int mbox_ambiq_data_read(uint32_t *value)
{
	if (am_hal_ipc_mbox_data_read(value, 1) != AM_HAL_STATUS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int mbox_ambiq_data_write(uint32_t value)
{
	if (am_hal_ipc_mbox_data_write(&value, 1) != AM_HAL_STATUS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int mbox_ambiq_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	int ret = -ENOSYS;
	uint32_t data32;

	if (channel >= MBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	if (msg == NULL) {
		/* Sometimes the mailbox is used for signalling with (msg == NULL). We still
		 * need to write the expected signalling number to the mailbox fifo to
		 * trigger the mailbox interrupt to the peer core.
		 */
		if (mbox_signal_magic[channel] == MBOX_INVALID_MAGIC) {
			return -EBADMSG;
		}
		data32 = mbox_signal_magic[channel];
	} else {
		if (msg->size != MBOX_MSG_SIZE) {
			return -EMSGSIZE;
		}
		memcpy(&data32, msg->data, msg->size);
	}

	ret = mbox_ambiq_data_write(data32);

	return ret;
}

static void mbox_ambiq_data_rx_handler(const struct device *dev)
{
	struct mbox_ambiq_data_t *data = dev->data;
	struct mbox_msg read_msg = {0};
	uint32_t read_data = 0;
	int ret;
	int channel;

	ret = mbox_ambiq_data_read(&read_data);
	am_hal_ipc_mbox_interrupt_clear(AM_HAL_IPC_MBOX_INT_CHANNEL_THRESHOLD);
	if (ret) {
		return;
	}

	/* Search for the channel of interrupt happening and execute the
	 * corresponding callback.
	 */
	for (channel = 0; channel < MBOX_MAX_CHANNELS; channel++) {
		if (read_data == mbox_signal_magic[channel]) {
			if (data->mbox_cb[channel] != NULL) {
				read_msg.data = (void *)&read_data;
				read_msg.size = MBOX_MSG_SIZE;
				data->mbox_cb[channel](dev, channel, data->user_data[channel],
						       &read_msg);
				return;
			}
		}
	}

#if (CONFIG_DT_HAS_VND_MBOX_CONSUMER_ENABLED)
	const struct mbox_dt_spec vnd_mbox_rx = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);

	channel = vnd_mbox_rx.channel_id;
	if (data->mbox_cb[channel] != NULL) {
		read_msg.data = (void *)&read_data;
		read_msg.size = MBOX_MSG_SIZE;
		data->mbox_cb[channel](dev, channel, data->user_data[channel], &read_msg);
	}
#endif /* CONFIG_DT_HAS_VND_MBOX_CONSUMER_ENABLED */
}

static void mbox_ambiq_error_handler(const struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t ui32Status;

	/* Reinitialize the mailbox if knows the peer is reinitialized */
	if (AM_HAL_IPC_MBOX_ERROR_IPCINIT) {
		am_hal_ipc_mbox_init_state_set(AM_HAL_IPC_MBOX_INIT_STATE_IPCINIT_RECEIVED);
		am_hal_ipc_mbox_init();
		return;
	}

	am_hal_ipc_mbox_error_status_get(&ui32Status);
	am_hal_ipc_mbox_error_clear(ui32Status);
	am_hal_ipc_mbox_interrupt_clear(AM_HAL_IPC_MBOX_INT_CHANNEL_ERROR);
}

static int mbox_ambiq_register_callback(const struct device *dev, uint32_t channel,
					mbox_callback_t cb, void *user_data)
{
	struct mbox_ambiq_data_t *data = dev->data;

	if (channel >= MBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	data->mbox_cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
}

static int mbox_ambiq_mtu_get(const struct device *dev)
{
	return MBOX_MSG_SIZE;
}

static uint32_t mbox_ambiq_max_channels_get(const struct device *dev)
{
	return MBOX_MAX_CHANNELS;
}

static int mbox_ambiq_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	int ret = 0;
	struct mbox_ambiq_data_t *data = dev->data;

	if (channel >= MBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	if ((enable == 0 && (!(data->enabled_mask & BIT(channel)))) ||
	    (enable != 0 && (data->enabled_mask & BIT(channel)))) {
		return -EALREADY;
	}

	if (enable && (data->mbox_cb[channel] == NULL)) {
		LOG_WRN("Enabling channel without a registered callback\n");
	}

	if (enable && data->enabled_mask == 0) {
		am_hal_ipc_mbox_interrupt_configure(AM_HAL_IPC_MBOX_INT_CTRL_ENABLE,
						    AM_HAL_IPC_MBOX_INT_CHANNEL_THRESHOLD);
		NVIC_ClearPendingIRQ(DT_INST_IRQ_BY_IDX(0, 0, irq));
		irq_enable(DT_INST_IRQ_BY_IDX(0, 0, irq));
		am_hal_ipc_mbox_interrupt_configure(AM_HAL_IPC_MBOX_INT_CTRL_ENABLE,
						    AM_HAL_IPC_MBOX_INT_CHANNEL_ERROR);
		NVIC_ClearPendingIRQ(DT_INST_IRQ_BY_IDX(0, 1, irq));
		irq_enable(DT_INST_IRQ_BY_IDX(0, 1, irq));
	}

	if (enable) {
		data->enabled_mask |= BIT(channel);
	} else {
		data->enabled_mask &= ~BIT(channel);
	}

	if (data->enabled_mask == 0) {
		irq_disable(DT_INST_IRQ_BY_IDX(0, 0, irq));
		am_hal_ipc_mbox_interrupt_configure(AM_HAL_IPC_MBOX_INT_CTRL_DISABLE,
						    AM_HAL_IPC_MBOX_INT_CHANNEL_THRESHOLD);
		irq_disable(DT_INST_IRQ_BY_IDX(0, 1, irq));
		am_hal_ipc_mbox_interrupt_configure(AM_HAL_IPC_MBOX_INT_CTRL_DISABLE,
						    AM_HAL_IPC_MBOX_INT_CHANNEL_ERROR);
	}

	return ret;
}

static int mbox_ambiq_init(const struct device *dev)
{
	struct mbox_ambiq_data_t *data = (struct mbox_ambiq_data_t *)dev->data;

	memcpy(&ambiq_mbox_data, data, sizeof(struct mbox_ambiq_data_t));

	/* Connect the interrupt handler */
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 0, irq), DT_INST_IRQ_BY_IDX(0, 0, priority),
		    mbox_ambiq_data_rx_handler, DEVICE_DT_INST_GET(0), 0);
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 1, irq), DT_INST_IRQ_BY_IDX(0, 1, priority),
		    mbox_ambiq_error_handler, DEVICE_DT_INST_GET(0), 0);

	return 0;
}

static struct mbox_driver_api mbox_ambiq_driver_api = {
	.send = mbox_ambiq_send,
	.register_callback = mbox_ambiq_register_callback,
	.mtu_get = mbox_ambiq_mtu_get,
	.max_channels_get = mbox_ambiq_max_channels_get,
	.set_enabled = mbox_ambiq_set_enabled,
};

DEVICE_DT_INST_DEFINE(0, mbox_ambiq_init, NULL, &ambiq_mbox_data, NULL, PRE_KERNEL_1,
		      CONFIG_MBOX_INIT_PRIORITY, &mbox_ambiq_driver_api);
