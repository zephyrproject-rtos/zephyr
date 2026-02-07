/*
 * Copyright (c) 2025 Igalia S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
/* pico-sdk includes */
#include <hardware/structs/sio.h>


#define DT_DRV_COMPAT raspberrypi_pico_mbox

LOG_MODULE_REGISTER(mbox_rpi_pico, CONFIG_MBOX_LOG_LEVEL);

#define MAILBOX_MBOX_SIZE sizeof(uint32_t)
#define MAILBOX_DEV_NAME mbox0

struct rpi_pico_mailbox_data {
	const struct device *dev;
	mbox_callback_t cb;
	void *user_data;
};

static struct rpi_pico_mailbox_data rpi_pico_mbox_data;


/*
 * Clears the ROE and WOF flags, if set.
 */
static inline void fifo_clear_status(void)
{
	sio_hw->fifo_st = 0xff;
}

/*
 * Returns true if the write FIFO isn't full. Returns false otherwise.
 */
static inline bool fifo_write_ready(void)
{
	return sio_hw->fifo_st & SIO_FIFO_ST_RDY_BITS;
}

/*
 * Returns true if the read FIFO has data available, ie. sent by the
 * other core. Returns false otherwise.
 */
static inline bool fifo_read_valid(void)
{
	return sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS;
}

/*
 * Discard any data in the read FIFO.
 */
static inline void fifo_drain(void)
{
	while (fifo_read_valid()) {
		(void)sio_hw->fifo_rd;
	}
}

static int rpi_pico_mbox_send(const struct device *dev,
				mbox_channel_id_t channel,
				const struct mbox_msg *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	if (!fifo_write_ready()) {
		return -EBUSY;
	}
	/* Signalling mode: send 0 as dummy data. */
	if (msg == NULL) {
		LOG_DBG("CPU %d: send IP signal", sio_hw->cpuid);
		sio_hw->fifo_wr = 0;
		__SEV();
		return 0;
	}

	if (msg->size > MAILBOX_MBOX_SIZE) {
		return -EMSGSIZE;
	}
	LOG_DBG("CPU %d: send IP data: %d", sio_hw->cpuid, *((int *)msg->data));
	sio_hw->fifo_wr = *((uint32_t *)(msg->data));
	__SEV();

	return 0;
}

static int rpi_pico_mbox_register_callback(const struct device *dev,
					mbox_channel_id_t channel,
					mbox_callback_t cb,
					void *user_data)
{
	ARG_UNUSED(channel);

	struct rpi_pico_mailbox_data *data = dev->data;
	uint32_t key;

	key = irq_lock();
	data->cb = cb;
	data->user_data = user_data;
	irq_unlock(key);

	return 0;
}

static int rpi_pico_mbox_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MAILBOX_MBOX_SIZE;
}

static uint32_t rpi_pico_mbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Only one channel per CPU supported. */
	return 1;
}

static int rpi_pico_mbox_set_enabled(const struct device *dev,
				mbox_channel_id_t channel, bool enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	if (enable) {
		irq_enable(DT_INST_IRQ_BY_NAME(0, MAILBOX_DEV_NAME, irq));
	} else {
		irq_disable(DT_INST_IRQ_BY_NAME(0, MAILBOX_DEV_NAME, irq));
	}

	return 0;
}

static void rpi_pico_mbox_isr(const struct device *dev)
{
	struct rpi_pico_mailbox_data *data = dev->data;

	/*
	 * Ignore the interrupt if it was triggered by anything that's
	 * not a FIFO receive event.
	 *
	 * NOTE: the interrupt seems to be triggered when it's first
	 * enabled even when the FIFO is empty.
	 */
	if (!fifo_read_valid()) {
		LOG_DBG("Interrupt received on empty FIFO: ignored.");
		return;
	}

	if (data->cb != NULL) {
		uint32_t d = sio_hw->fifo_rd;
		struct mbox_msg msg = {
			.data = &d,
			.size = sizeof(d)};
		data->cb(dev, 0, data->user_data, &msg);
	}
	fifo_drain();
}

static int rpi_pico_mbox_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("Initial FIFO status: 0x%x", sio_hw->fifo_st);
	LOG_DBG("FIFO depth: %d", DT_INST_PROP(0, fifo_depth));
	irq_disable(DT_INST_IRQ_BY_NAME(0, MAILBOX_DEV_NAME, irq));
	fifo_drain();
	fifo_clear_status();
	LOG_DBG("FIFO status after setup: 0x%x", sio_hw->fifo_st);
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, MAILBOX_DEV_NAME, irq),
		DT_INST_IRQ_BY_NAME(0, MAILBOX_DEV_NAME, priority),
		rpi_pico_mbox_isr, DEVICE_DT_INST_GET(0), 0);

	return 0;
}

static DEVICE_API(mbox, rpi_pico_mbox_driver_api) = {
	.send = rpi_pico_mbox_send,
	.register_callback = rpi_pico_mbox_register_callback,
	.mtu_get = rpi_pico_mbox_mtu_get,
	.max_channels_get = rpi_pico_mbox_max_channels_get,
	.set_enabled = rpi_pico_mbox_set_enabled,
};

DEVICE_DT_INST_DEFINE(
	0,
	rpi_pico_mbox_init,
	NULL,
	&rpi_pico_mbox_data,
	NULL,
	POST_KERNEL,
	CONFIG_MBOX_INIT_PRIORITY,
	&rpi_pico_mbox_driver_api);
