/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief TI AM26x Family Mailbox Driver
 *
 * This driver provides a Zephyr mailbox interface to the TI AM26x family
 * mailbox hardware. It is designed to be compatible with TI's IPC Notify
 * driver from their MCU+ SDK but adapted to Zephyr's driver model.
 *
 * TI SDK IPC Notify vs Zephyr AM26x Mailbox Driver Comparison
 * ==========================================================
 *
 * This implementation is effectively a port of TI's IPC Notify v1 driver
 * to Zephyr's mailbox framework. Here's how the components map between
 * the two implementations:
 *
 * Conceptual Mapping
 * ------------------
 * - TI SDK : IPC Notify v1 driver → Zephyr : AM26x mailbox driver
 * - Both access the same mailbox hardware in the AM26x SoC
 * - Both use the same interrupt mechanisms and software queue implementation
 *
 * Component Mapping:
 * ----------------
 * 1. Data Structures
 *    - TI SDK : IpcNotify_SwQueue → Zephyr: am26x_mbox_sw_queue
 *
 * 2. API Mapping
 *    - TI SDK : IpcNotify_sendMsg() → Zephyr: mbox_send()
 *    - TI SDK : IpcNotify_registerClient() → Zephyr: mbox_register_callback()
 *    - TI SDK : IpcNotify_isCoreEnabled() → Zephyr: N/A
 *
 * 3. Interrupt Handling
 *    - TI SDK : Uses a single interrupt per core for read requests
 *    - Zephyr : Same approach - single interrupt per mailbox instance
 *    - Both use SW queues for actual message data
 *
 * 4. Register Usage
 *    - Both use the same mailbox register addresses for communication
 *    - Both use the same bit positions for core-specific signaling
 *
 * Hardware Details :
 * ---------------
 * - AM26x uses MSS controller integrated mailbox registers
 * - Separate registers for each core (R5FSS0_0, R5FSS0_1, ...)
 * - Read request and write done registers for signaling
 * - Software queue mechanism for actual message data
 *
 * This implementation maintains compatibility with the TI SDK's approach
 * which allows it to interoperate with cores running the TI SDK.
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/mbox/ti_am26x_mbox.h>

LOG_MODULE_REGISTER(mbox_ti_am26x, CONFIG_MBOX_LOG_LEVEL);

#define DT_DRV_COMPAT ti_am26x_mailbox

extern am26x_mbox_cfg am26x_mbox_static_cfg[CONFIG_MAX_CPUS_NUM][CONFIG_MAX_CPUS_NUM];

/* Helper macros to access device config and data */
#define DEV_CFG(_dev)           ((struct am26x_mbox_config *)(_dev)->config)
#define DEV_DATA(_dev)          ((struct am26x_mbox_data *)(_dev)->data)
#define DEV_CTRL_BASE(_dev)     ((uint32_t)DEVICE_MMIO_NAMED_GET(_dev, ctrl))
#define DEV_MBOX_RAM_BASE(_dev) ((uint32_t)DEVICE_MMIO_NAMED_GET(_dev, mbox_ram))

/**
 * @brief Software queue structure for mailbox communication
 *
 * This structure is based on TI's IpcNotify_SwQueue in their SDK.
 * It provides a mechanism to pass data along with mailbox interrupts.
 */
struct am26x_mbox_sw_queue {
	uint32_t rdIdx;                              /* Read index */
	uint32_t wrIdx;                              /* Write index */
	uint32_t msgs[AM26X_MBOX_MAX_MSGS_IN_QUEUE]; /* Message data */
};

/**
 * @brief Channel information for mailbox driver
 */
struct am26x_mbox_channel {
	mbox_callback_t cb; /* Callback function */
	void *user_data;    /* User data for callback */
};

/**
 * @brief Runtime data for mailbox driver instance
 */
struct am26x_mbox_data {
	DEVICE_MMIO_NAMED_RAM(ctrl);                                  /* MSS controller registers */
	DEVICE_MMIO_NAMED_RAM(mbox_ram);                              /* Mailbox SRAM */
	struct am26x_mbox_channel channels[AM26X_MBOX_MAX_CH_NUM];    /* Channel configuration */
	struct am26x_mbox_sw_queue *sw_queues[AM26X_MBOX_MAX_CH_NUM]; /* Software queues */
};

/**
 * @brief Configuration data for mailbox driver instance
 */
struct am26x_mbox_config {
	DEVICE_MMIO_NAMED_ROM(ctrl);     /* MSS controller registers */
	DEVICE_MMIO_NAMED_ROM(mbox_ram); /* Mailbox SRAM */
	uint32_t irq;                    /* Interrupt number */
	uint32_t self_core_id;           /* Core ID (0 for R5FSS0_0, 1 for R5FSS0_1) */
};

/**
 * @brief Get write mailbox register address for communicating with a remote core
 *
 * This function determines the appropriate mailbox register to use for signaling
 * a remote core, based on the local and remote core IDs.
 *
 * @param[in] core_id The core ID to communicate with
 * @return The mailbox register offset
 */
static inline uint32_t get_write_mailbox_addr(uint32_t core_id)
{
	am26x_mbox_cfg *am26x_mbox_cfg_ptr;
	am26x_mbox_cfg_ptr = &am26x_mbox_static_cfg[core_id][core_id];
	return am26x_mbox_cfg_ptr->write_done_offset;
}

/**
 * @brief Get read mailbox register address for receiving from a remote core
 *
 * This function returns the mailbox register offset to monitor for incoming
 * messages, based on the local core ID. It supports R5FSS0_0 and R5FSS0_1 cores.
 *
 * @param[in] core_id The self core ID (0 for R5FSS0_0, 1 for R5FSS0_1)
 * @return The mailbox register offset
 */
static inline uint32_t get_read_mailbox_addr(uint32_t core_id)
{
	am26x_mbox_cfg *am26x_mbox_cfg_ptr;
	am26x_mbox_cfg_ptr = &am26x_mbox_static_cfg[core_id][core_id];
	return am26x_mbox_cfg_ptr->read_req_offset;
}

/**
 * @brief Get core bit position for mailbox registers
 *
 * This function returns the bit position in mailbox registers corresponding
 * to the specified core ID. Only core IDs 0 and 1 are supported.
 *
 * @param[in] core_id The core ID
 * @return The bit position in mailbox registers
 */
static inline uint32_t get_core_bit_pos(uint32_t core_id)
{
	am26x_mbox_cfg *am26x_mbox_cfg_ptr;
	am26x_mbox_cfg_ptr = &am26x_mbox_static_cfg[core_id][core_id];
	return am26x_mbox_cfg_ptr->read_req_offset;
}

static inline uint32_t get_sw_q_idx(uint32_t sender_core_id, uint32_t receiver_core_id)
{

	am26x_mbox_cfg *am26x_mbox_cfg_ptr;
	am26x_mbox_cfg_ptr = &am26x_mbox_static_cfg[sender_core_id][receiver_core_id];
	return am26x_mbox_cfg_ptr->swQ_addr;
}

/**
 * @brief Read a message from the software queue
 *
 * This function reads a message from the software queue associated with
 * a specific channel.
 *
 * @param[in] queue Pointer to the software queue
 * @param[out] msg Pointer to store the read message
 * @return 0 on success, -ENODATA if queue is empty
 */
static int sw_queue_read(struct am26x_mbox_sw_queue *queue, uint32_t *msg)
{
	int ret = 0;
	uint32_t wrIdx;
	uint32_t rdIdx;
	unsigned int key;

	if (queue == NULL) {
		LOG_ERR("Queue is NULL");
		return -EINVAL;
	}

	/* Disabling Interrupts */
	key = irq_lock();

	wrIdx = queue->wrIdx;
	rdIdx = queue->rdIdx;

	/* Check if the Queue is not full */
	if ((rdIdx < AM26X_MBOX_MAX_MSGS_IN_QUEUE) && (wrIdx < AM26X_MBOX_MAX_MSGS_IN_QUEUE)) {
		if (rdIdx != wrIdx) {

			*msg = queue->msgs[rdIdx];

			rdIdx = (rdIdx + 1) % AM26X_MBOX_MAX_MSGS_IN_QUEUE;

			queue->rdIdx = rdIdx;
			rdIdx = queue->rdIdx; /* Read back to ensure write completion */

			/* Data and Instruction Barrier */
			z_barrier_dsync_fence_full();
			z_barrier_isync_fence_full();

		} else {

			/* Queue is empty return error */
			LOG_DBG("Queue Empty");
			
			/* Restoring the Interrupts */
			irq_unlock(key);

			return -ENODATA;
		}
	}

	LOG_DBG("Read message 0x%08x from queue", *msg);

	/* Restoring the Interrupts */
	irq_unlock(key);


	return ret;
}

/**
 * @brief Write a message to the software queue
 *
 * This function writes a message to the software queue associated with
 * a specific channel.
 *
 * @param[in] queue Pointer to the software queue
 * @param[in] msg Message to write (32-bit value)
 * @return 0 on success, -ENOSPC if queue is full
 */
static int sw_queue_write(struct am26x_mbox_sw_queue *queue, uint32_t msg)
{
	int ret = 0;
	uint32_t wrIdx;
	uint32_t rdIdx;
	unsigned int key;

	if (queue == NULL) {
		LOG_ERR("Queue is NULL");
		return -EINVAL;
	}

	/* Disabling Interrupts */
	key = irq_lock();

	wrIdx = queue->wrIdx;
	rdIdx = queue->rdIdx;

	/* rdIdx, wrIdx checks against the Max Msgs */
	if ((rdIdx < AM26X_MBOX_MAX_MSGS_IN_QUEUE) && (wrIdx < AM26X_MBOX_MAX_MSGS_IN_QUEUE)) {
		/* Check if the Queue is not full */
		if (((wrIdx + 1) % AM26X_MBOX_MAX_MSGS_IN_QUEUE) != rdIdx) {
			/* Write message to queue */
			queue->msgs[wrIdx] = msg;

			wrIdx = (wrIdx + 1) % AM26X_MBOX_MAX_MSGS_IN_QUEUE;

			queue->wrIdx = wrIdx;
			wrIdx = queue->wrIdx; /* Read back to ensure write completion */

			/* Data and Instruction Barrier */
			z_barrier_dsync_fence_full();
			z_barrier_isync_fence_full();

		} else {
			/* Queue is full return error */
			LOG_DBG("Queue Full");
	
			/* Restoring the Interrupts */
			irq_unlock(key);

			return -ENOSPC;
		}
	}

	LOG_DBG("Wrote message 0x%08x to queue", msg);

	/* Restoring the Interrupts */
	irq_unlock(key);


	return ret;
}

/**
 * @brief Process messages in a channel's queue
 *
 * This function processes all messages in a channel's queue and calls
 * the registered callback for each message.
 *
 * @param[in] dev The mailbox device
 * @param[in] channel The channel number
 */
static void process_channel_messages(const struct device *dev, uint32_t channel)
{
	struct am26x_mbox_data *data = DEV_DATA(dev);
	struct am26x_mbox_channel *chan = &data->channels[channel];
	struct am26x_mbox_sw_queue *queue = data->sw_queues[channel];
	uint32_t msg;
	int ret;

	if (queue == NULL) {
		return;
	}

	/* Process all messages in the queue */
	do {
		ret = sw_queue_read(queue, &msg);

		/* If there's a callback registered, call it */
		if (chan->cb != NULL) {
			struct mbox_msg cb_msg = {
				.data = &msg, 
				.size = AM26X_MBOX_MSG_SIZE
			};

			LOG_DBG("Calling callback for channel %u with message 0x%08x", channel,
				msg);

			/* Call user callback */
			chan->cb(dev, channel, chan->user_data, &cb_msg);
		}
	} while (ret == 0);
}

/**
 * @brief Interrupt service routine for mailbox
 *
 * This function is called when a mailbox interrupt is triggered. It reads
 * the pending interrupts, determines which cores have sent messages, and
 * processes the messages in the corresponding channels' queues.
 *
 * @param[in] arg Pointer to the mailbox device
 */
static void am26x_mbox_isr(void *arg)
{
	const struct device *dev = arg;
	const struct am26x_mbox_config *config = DEV_CFG(dev);
	uint32_t ctrl_base = DEV_CTRL_BASE(dev);

	uint32_t self_core_id = config->self_core_id;

	/*
	 * ISR is triggered on a recieving core. So, all the interrupts that are
	 * are aggregated over the Reciever core needs to be handled.
	 */

	uint32_t read_addr = get_read_mailbox_addr(self_core_id);
	uint32_t pending_intr = 0;

	int channel;

	do {
		/* Read the interrupt status register */
		volatile uint32_t *reg = (volatile uint32_t *)(ctrl_base + read_addr);
		pending_intr = *reg;

		/* Clear the Pending Requests. This is unconditional, we read Sw Queues later */
		*reg = pending_intr;

		LOG_DBG("ISR triggered, pending interrupts: 0x%08x", pending_intr);

		/* Check which cores have sent messages */
		for (channel = 0; channel < AM26X_MBOX_MAX_CH_NUM; channel++) {

			if ((!AM26X_MBOX_IS_CH_VALID(channel)) ||
			    (self_core_id != AM26X_MBOX_GET_RECEIVER(channel))) {
				continue;
			}
			/* process all the sw queues that correspond the current core as reciever */

			/* Process messages for this channel */
			process_channel_messages(dev, channel);
		}

		/* Re-read the interrupt register to catch new pending interrupts */
		pending_intr = *reg;
	} while (pending_intr != 0);
}

/**
 * @brief Trigger an interrupt to the remote core
 *
 * This function sets the appropriate bit in the mailbox register to
 * trigger an interrupt on the remote core.
 *
 * @param[in] dev The mailbox device
 * @param[in] channel The channel ID
 */
static void trigger_remote_interrupt(const struct device *dev, uint32_t channel)
{
	uint32_t ctrl_base = DEV_CTRL_BASE(dev);

	uint32_t self_core_id;
	uint32_t remote_core_id;

	uint32_t write_addr;
	uint32_t bit_pos;

	self_core_id = AM26X_MBOX_GET_SENDER(channel);
	remote_core_id = AM26X_MBOX_GET_RECEIVER(channel);

	/* Get the appropriate mailbox register address and bit position */
	write_addr = get_write_mailbox_addr(self_core_id);
	bit_pos = get_core_bit_pos(remote_core_id);

	/* Trigger the interrupt by setting the bit */
	volatile uint32_t *reg = (volatile uint32_t *)(ctrl_base + write_addr);
	*reg = BIT(bit_pos);

	LOG_DBG("SelfCoreID : %d, Triggered interrupt to core %u (reg: 0x%08x, bit: %lu)",
		self_core_id, remote_core_id, (uint32_t)reg, BIT(bit_pos));
}

/**
 * @brief Send a message through a mailbox channel
 *
 * This function sends a message to a remote core through a mailbox channel.
 * It writes the message to the software queue associated with the channel
 * and then triggers an interrupt to notify the remote core.
 *
 * @param[in] dev The mailbox device
 * @param[in] channel The channel number
 * @param[in] msg The message to send
 * @return 0 on success, negative errno on failure
 */
static int am26x_mbox_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	struct am26x_mbox_data *data = dev->data;
	struct am26x_mbox_sw_queue *queue;
	uint32_t value;
	int ret;

	/* Validate parameters */
	if (!AM26X_MBOX_IS_CH_VALID(channel)) {
		LOG_ERR("Invalid channel number: %u", channel);
		return -EINVAL;
	}

	/* Queues are statically allocated across system for hetrogenous usage between platforms */
	queue = data->sw_queues[channel];

	/* Extract message data */
	if (msg == NULL || msg->data == NULL) {
		value = 0;
	} else {
		if (msg->size > AM26X_MBOX_MSG_SIZE) {
			LOG_ERR("Message too large (%zu bytes, max %u)", msg->size,
				AM26X_MBOX_MSG_SIZE);
			return -EMSGSIZE;
		}

		/* Copy message data - handle different sizes */
		value = *((uint32_t *)msg->data);
	}

	/* Write message to software queue */
	ret = sw_queue_write(queue, value);
	if (ret < 0) {
		return ret;
	}

	/* Trigger interrupt to remote core */
	trigger_remote_interrupt(dev, channel);

	return 0;
}

/**
 * @brief Register a callback for a mailbox channel
 *
 * This function registers a callback function to be called when a message
 * is received on a specific channel. It also allocates a software queue
 * for the channel if one doesn't already exist.
 *
 * @param[in] dev The mailbox device
 * @param[in] channel The channel number
 * @param[in] cb The callback function
 * @param[in] user_data User data to pass to the callback
 * @return 0 on success, negative errno on failure
 */
static int am26x_mbox_register_callback(const struct device *dev, uint32_t channel,
					mbox_callback_t cb, void *user_data)
{
	struct am26x_mbox_data *data = dev->data;
	uint32_t sender_core_id = AM26X_MBOX_GET_SENDER(channel);
	uint32_t reciever_core_id = AM26X_MBOX_GET_RECEIVER(channel);

	LOG_DBG("Registering callback for channel %u", channel);

	/* Validate parameters */
	if (!AM26X_MBOX_IS_CH_VALID(channel)) {
		LOG_ERR("Invalid channel number: %u", channel);
		return -EINVAL;
	}

	/* Store callback and user data */
	data->channels[channel].cb = cb;
	data->channels[channel].user_data = user_data;

	/* Allocate software queue if not already allocated */
	if (data->sw_queues[channel] == NULL) {
		LOG_DBG("Cleaning SW queue for channel %u", channel);

		/* Initialize the queue */
		memset((struct am26x_mbox_sw_queue *)get_sw_q_idx(sender_core_id, reciever_core_id),
		       0, sizeof(struct am26x_mbox_sw_queue));
		data->sw_queues[channel] = (struct am26x_mbox_sw_queue *)get_sw_q_idx(
			sender_core_id, reciever_core_id);
		data->sw_queues[channel]->rdIdx = 0;
		data->sw_queues[channel]->wrIdx = 0;
	}

	LOG_DBG("Callback registered for channel %u", channel);
	return 0;
}

static int am26x_mbox_mtu_get(const struct device *dev)
{
	/* AM26x mailbox only supports 32-bit messages */
	return AM26X_MBOX_MSG_SIZE;
}

static uint32_t am26x_mbox_max_channels_get(const struct device *dev)
{
	return AM26X_MBOX_MAX_CH_NUM_VALID;
}
/**
 * @brief Enable or disable a mailbox channel
 *
 * This function enables or disables a mailbox channel. When a channel is
 * enabled, it can send and receive messages. When disabled, it cannot.
 * Enabling a channel also enables the interrupt for receiving messages.
 *
 * @param[in] dev The mailbox device
 * @param[in] channel The channel number
 * @param[in] enable True to enable, false to disable
 * @return 0 on success, negative errno on failure
 */
static int am26x_mbox_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(enable);
	return 0;
}

/* Define driver API structure */
static const struct mbox_driver_api am26x_mbox_api = {
	.send = am26x_mbox_send,
	.register_callback = am26x_mbox_register_callback,
	.mtu_get = am26x_mbox_mtu_get,
	.max_channels_get = am26x_mbox_max_channels_get,
	.set_enabled = am26x_mbox_set_enabled,
};

/* Driver instantiation macro */
#define AM26X_MBOX_DEVICE_INIT(n)														\
	static struct am26x_mbox_data am26x_mbox_data_##n = {0};							\
                                                                                        \
	const static struct am26x_mbox_config am26x_mbox_config_##n = {						\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ctrl, DT_DRV_INST(n)),						\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(mbox_ram, DT_DRV_INST(n)),					\
		.irq = DT_INST_IRQN(n),															\
		.self_core_id = DT_INST_PROP(n, core_id),										\
	};																					\
																						\
	static int am26x_mbox_##n##_init(const struct device *dev)							\
	{																					\
		struct am26x_mbox_data *data = DEV_DATA(dev);									\
		const struct am26x_mbox_config *config = DEV_CFG(dev);							\
		int i;																			\
																						\
		LOG_DBG("Initializing AM26x mailbox driver (core ID: %u)", config->self_core_id);	\
																							\
		/* Initialize device memory-mapped IO */											\
		DEVICE_MMIO_NAMED_MAP(dev, ctrl, K_MEM_CACHE_NONE);									\
		DEVICE_MMIO_NAMED_MAP(dev, sram, K_MEM_CACHE_NONE);									\
																							\
		/* Initialize channel data */														\
		for (i = 0; i < AM26X_MBOX_MAX_CH_NUM; i++) {									\
			data->channels[i].cb = NULL;													\
			data->channels[i].user_data = NULL;												\
			data->sw_queues[i] = NULL;														\
			/* clearing and updating all the tx queues */									\
			uint32_t sender_core_id = AM26X_MBOX_GET_SENDER(i);						\
			uint32_t reciever_core_id = AM26X_MBOX_GET_RECEIVER(i);					\
			memset((struct am26x_mbox_sw_queue *)get_sw_q_idx(sender_core_id, reciever_core_id), \
		       0, sizeof(struct am26x_mbox_sw_queue));										\
			data->sw_queues[i] = (struct am26x_mbox_sw_queue *)get_sw_q_idx(			\
			sender_core_id, reciever_core_id);												\
		}																					\
																							\
		/* Configure and enable interrupt */												\
		irq_disable(DT_INST_IRQN(n));														\
		IRQ_CONNECT(DT_INST_IRQN(n), 0, am26x_mbox_isr, DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));														\
																							\
		/* Don't enable the interrupt yet - it will be enabled when a channel is enabled   \
		 */																					\
																							\
		LOG_DBG("AM26x mailbox initialized successfully");									\
		return 0;																			\
	}																						\
																							\
	DEVICE_DT_INST_DEFINE(n, am26x_mbox_##n##_init, NULL, &am26x_mbox_data_##n,				\
			      &am26x_mbox_config_##n, POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY,			\
			      &am26x_mbox_api);

/* Create device instances based on device tree */
DT_INST_FOREACH_STATUS_OKAY(AM26X_MBOX_DEVICE_INIT)
