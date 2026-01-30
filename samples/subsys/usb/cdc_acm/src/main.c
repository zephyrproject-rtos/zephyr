/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdc_acm_echo, LOG_LEVEL_INF);

#define RING_BUF_SIZE 1024

/*
 * Per-instance data for each CDC-ACM UART device.
 * This allows supporting multiple CDC-ACM instances simultaneously.
 * Each instance has its own thread to handle connections independently.
 */
struct cdc_acm_instance {
	const struct device *uart_dev;
	uint8_t ring_buffer[RING_BUF_SIZE];
	struct ring_buf ringbuf;
	struct k_sem dtr_sem;
	struct k_thread thread;
	k_thread_stack_t *stack;
	size_t idx;
	bool rx_throttled;
	bool initialized;
};

#define CDC_ACM_STACK_SIZE 1024

/* Define stack for each instance */
#define CDC_ACM_STACK_DEFINE(node_id)					\
	K_THREAD_STACK_DEFINE(cdc_acm_stack_##node_id, CDC_ACM_STACK_SIZE);

DT_FOREACH_STATUS_OKAY(zephyr_cdc_acm_uart, CDC_ACM_STACK_DEFINE)

/* Define instance data for each CDC-ACM UART device */
#define CDC_ACM_INSTANCE_DEFINE(node_id)				\
	{								\
		.uart_dev = DEVICE_DT_GET(node_id),			\
		.dtr_sem = Z_SEM_INITIALIZER(				\
			cdc_acm_instances[DT_NODE_CHILD_IDX(node_id)].dtr_sem, 0, 1), \
		.stack = cdc_acm_stack_##node_id,			\
		.idx = DT_NODE_CHILD_IDX(node_id),			\
	},

static struct cdc_acm_instance cdc_acm_instances[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_cdc_acm_uart, CDC_ACM_INSTANCE_DEFINE)
};

#define CDC_ACM_INSTANCE_COUNT ARRAY_SIZE(cdc_acm_instances)

static inline void print_baudrate(const struct device *dev)
{
	uint32_t baudrate;
	int ret;

	ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		LOG_WRN("Failed to get baudrate, ret code %d", ret);
	} else {
		LOG_INF("Baudrate %u", baudrate);
	}
}

/*
 * Find instance by UART device pointer.
 */
static struct cdc_acm_instance *find_instance_by_dev(const struct device *dev)
{
	for (size_t i = 0; i < CDC_ACM_INSTANCE_COUNT; i++) {
		if (cdc_acm_instances[i].uart_dev == dev) {
			return &cdc_acm_instances[i];
		}
	}
	return NULL;
}

static void sample_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}

	if (msg->type == USBD_MSG_CDC_ACM_CONTROL_LINE_STATE) {
		uint32_t dtr = 0U;
		struct cdc_acm_instance *inst = find_instance_by_dev(msg->dev);

		uart_line_ctrl_get(msg->dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr && inst != NULL) {
			/* Signal instance semaphore to unblock its thread */
			k_sem_give(&inst->dtr_sem);
		}
	}

	if (msg->type == USBD_MSG_CDC_ACM_LINE_CODING) {
		print_baudrate(msg->dev);
	}
}

static int enable_usb_device_next(void)
{
	int initialized;
	size_t device_count = sample_usbd_get_device_count();

	LOG_INF("Discovered %zu UDC device(s) via zephyr,udc compatible",
		device_count);

	initialized = sample_usbd_init_all_devices(sample_msg_cb);
	if (initialized == 0) {
		LOG_ERR("Failed to initialize any USB device");
		return -ENODEV;
	}

	LOG_INF("Initialized %d USB device(s)", initialized);

	/* Enable devices that cannot detect VBUS */
	for (size_t i = 0; i < device_count; i++) {
		struct usbd_context *ctx = sample_usbd_get_context(i);

		if (ctx != NULL && !usbd_can_detect_vbus(ctx)) {
			int err = usbd_enable(ctx);

			if (err) {
				LOG_ERR("Failed to enable device %zu", i);
			} else {
				LOG_INF("Enabled USB device %zu", i);
			}
		}
	}

	LOG_INF("USB device support enabled");

	return 0;
}

static void interrupt_handler(const struct device *dev, void *user_data)
{
	struct cdc_acm_instance *inst = user_data;

	if (inst == NULL) {
		return;
	}

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (!inst->rx_throttled && uart_irq_rx_ready(dev)) {
			int recv_len, rb_len;
			uint8_t buffer[64];
			size_t len = MIN(ring_buf_space_get(&inst->ringbuf),
					 sizeof(buffer));

			if (len == 0) {
				/* Throttle because ring buffer is full */
				uart_irq_rx_disable(dev);
				inst->rx_throttled = true;
				continue;
			}

			recv_len = uart_fifo_read(dev, buffer, len);
			if (recv_len < 0) {
				LOG_ERR("Failed to read UART FIFO");
				recv_len = 0;
			};

			rb_len = ring_buf_put(&inst->ringbuf, buffer, recv_len);
			if (rb_len < recv_len) {
				LOG_ERR("Drop %u bytes", recv_len - rb_len);
			}

			LOG_DBG("tty fifo -> ringbuf %d bytes", rb_len);
			if (rb_len) {
				uart_irq_tx_enable(dev);
			}
		}

		if (uart_irq_tx_ready(dev)) {
			uint8_t buffer[64];
			int rb_len, send_len;

			rb_len = ring_buf_get(&inst->ringbuf, buffer, sizeof(buffer));
			if (!rb_len) {
				LOG_DBG("Ring buffer empty, disable TX IRQ");
				uart_irq_tx_disable(dev);
				continue;
			}

			if (inst->rx_throttled) {
				uart_irq_rx_enable(dev);
				inst->rx_throttled = false;
			}

			send_len = uart_fifo_fill(dev, buffer, rb_len);
			if (send_len < rb_len) {
				LOG_ERR("Drop %d bytes", rb_len - send_len);
			}

			LOG_DBG("ringbuf -> tty fifo %d bytes", send_len);
		}
	}
}

static int init_cdc_acm_instance(struct cdc_acm_instance *inst, size_t idx)
{
	int ret;

	if (!device_is_ready(inst->uart_dev)) {
		LOG_ERR("CDC ACM device %zu not ready", idx);
		return -ENODEV;
	}

	ring_buf_init(&inst->ringbuf, sizeof(inst->ring_buffer),
		      inst->ring_buffer);
	inst->rx_throttled = false;
	inst->initialized = true;

	LOG_INF("CDC ACM instance %zu initialized (%s)", idx,
		inst->uart_dev->name);

	return 0;
}

static void setup_cdc_acm_instance(struct cdc_acm_instance *inst)
{
	int ret;

	/* They are optional, we use them to test the interrupt endpoint */
	ret = uart_line_ctrl_set(inst->uart_dev, UART_LINE_CTRL_DCD, 1);
	if (ret) {
		LOG_WRN("Instance %zu: Failed to set DCD, ret code %d",
			inst->idx, ret);
	}

	ret = uart_line_ctrl_set(inst->uart_dev, UART_LINE_CTRL_DSR, 1);
	if (ret) {
		LOG_WRN("Instance %zu: Failed to set DSR, ret code %d",
			inst->idx, ret);
	}

	uart_irq_callback_user_data_set(inst->uart_dev, interrupt_handler, inst);
	uart_irq_rx_enable(inst->uart_dev);

	LOG_INF("CDC ACM instance %zu ready for data", inst->idx);
}

/*
 * Per-instance thread function.
 * Each instance waits for its own DTR signal and then sets up the UART.
 */
static void cdc_acm_instance_thread(void *p1, void *p2, void *p3)
{
	struct cdc_acm_instance *inst = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Instance %zu: Waiting for DTR", inst->idx);

	/* Wait for DTR on this instance */
	k_sem_take(&inst->dtr_sem, K_FOREVER);

	LOG_INF("Instance %zu: DTR set", inst->idx);

	/* Wait 100ms for the host to do all settings */
	k_msleep(100);

	/* Setup this instance */
	setup_cdc_acm_instance(inst);
}

int main(void)
{
	int ret;
	size_t ready_count = 0;

	LOG_INF("CDC ACM Echo sample with %zu instance(s)", CDC_ACM_INSTANCE_COUNT);

	/* Initialize all CDC-ACM instances */
	for (size_t i = 0; i < CDC_ACM_INSTANCE_COUNT; i++) {
		ret = init_cdc_acm_instance(&cdc_acm_instances[i], i);
		if (ret == 0) {
			ready_count++;
		}
	}

	if (ready_count == 0) {
		LOG_ERR("No CDC ACM devices ready");
		return 0;
	}

	ret = enable_usb_device_next();
	if (ret != 0) {
		LOG_ERR("Failed to enable USB device support");
		return 0;
	}

	/* Spawn a thread for each initialized instance */
	for (size_t i = 0; i < CDC_ACM_INSTANCE_COUNT; i++) {
		struct cdc_acm_instance *inst = &cdc_acm_instances[i];

		if (!inst->initialized) {
			continue;
		}

		k_thread_create(&inst->thread, inst->stack,
				CDC_ACM_STACK_SIZE,
				cdc_acm_instance_thread,
				inst, NULL, NULL,
				K_PRIO_COOP(7), 0, K_NO_WAIT);

		k_thread_name_set(&inst->thread, inst->uart_dev->name);
	}

	LOG_INF("All instance threads started");

	return 0;
}
