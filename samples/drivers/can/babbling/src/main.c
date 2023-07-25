/*
 * Copyright (c) 2021-2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/* Devicetree */
#define CANBUS_NODE DT_CHOSEN(zephyr_canbus)
#define BUTTON_NODE DT_ALIAS(sw0)
#define BUTTON_NAME DT_PROP_OR(BUTTON_NODE, label, "sw0")

#if DT_NODE_EXISTS(BUTTON_NODE)
struct button_callback_context {
	struct gpio_callback callback;
	struct k_sem sem;
};

static void button_callback(const struct device *port, struct gpio_callback *cb,
			    gpio_port_pins_t pins)
{
	struct button_callback_context *ctx =
		CONTAINER_OF(cb, struct button_callback_context, callback);

	k_sem_give(&ctx->sem);
}
#endif /* DT_NODE_EXISTS(BUTTON_NODE) */

static void can_tx_callback(const struct device *dev, int error, void *user_data)
{
	struct k_sem *tx_queue_sem = user_data;

	k_sem_give(tx_queue_sem);
}

int main(void)
{
#if DT_NODE_EXISTS(BUTTON_NODE)
	const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
	struct button_callback_context btn_cb_ctx;
#endif /* DT_NODE_EXISTS(BUTTON_NODE) */
	const struct device *dev = DEVICE_DT_GET(CANBUS_NODE);
	struct k_sem tx_queue_sem;
	struct can_frame frame = {0};
	int err;

	k_sem_init(&tx_queue_sem, CONFIG_SAMPLE_CAN_BABBLING_TX_QUEUE_SIZE,
		   CONFIG_SAMPLE_CAN_BABBLING_TX_QUEUE_SIZE);

	if (!device_is_ready(dev)) {
		printk("CAN device not ready");
		return 0;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_CAN_BABBLING_FD_MODE)) {
		err = can_set_mode(dev, CAN_MODE_FD);
		if (err != 0) {
			printk("Error setting CAN-FD mode (err %d)", err);
			return 0;
		}
	}

	err = can_start(dev);
	if (err != 0) {
		printk("Error starting CAN controller (err %d)", err);
		return 0;
	}

#if DT_NODE_EXISTS(BUTTON_NODE)
	k_sem_init(&btn_cb_ctx.sem, 0, 1);

	if (!device_is_ready(btn.port)) {
		printk("button device not ready\n");
		return 0;
	}

	err = gpio_pin_configure_dt(&btn, GPIO_INPUT);
	if (err != 0) {
		printk("failed to configure button GPIO (err %d)\n", err);
		return 0;
	}

	err = gpio_pin_interrupt_configure_dt(&btn, GPIO_INT_EDGE_TO_ACTIVE);
	if (err != 0) {
		printk("failed to configure button interrupt (err %d)\n", err);
		return 0;
	}

	gpio_init_callback(&btn_cb_ctx.callback, button_callback, BIT(btn.pin));
	gpio_add_callback(btn.port, &btn_cb_ctx.callback);
#endif /* DT_NODE_EXISTS(BUTTON_NODE) */

	if (IS_ENABLED(CONFIG_SAMPLE_CAN_BABBLING_EXT_ID)) {
		frame.flags |= CAN_FRAME_IDE;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_CAN_BABBLING_RTR)) {
		frame.flags |= CAN_FRAME_RTR;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_CAN_BABBLING_FD_MODE)) {
		frame.flags |= CAN_FRAME_FDF;
	}

	frame.id = CONFIG_SAMPLE_CAN_BABBLING_CAN_ID;

	printk("babbling on %s with %s (%d-bit) CAN ID 0x%0*x, RTR %d, CAN-FD %d\n",
	       dev->name,
	       (frame.flags & CAN_FRAME_IDE) != 0 ? "extended" : "standard",
	       (frame.flags & CAN_FRAME_IDE) != 0 ? 29 : 11,
	       (frame.flags & CAN_FRAME_IDE) != 0 ? 8 : 3, frame.id,
	       (frame.flags & CAN_FRAME_RTR) != 0 ? 1 : 0,
	       (frame.flags & CAN_FRAME_FDF) != 0 ? 1 : 0);

#if DT_NODE_EXISTS(BUTTON_NODE)
	printk("abort by pressing %s button\n", BUTTON_NAME);
#endif /* DT_NODE_EXISTS(BUTTON_NODE) */

	while (true) {
		if (k_sem_take(&tx_queue_sem, K_MSEC(100)) == 0) {
			err = can_send(dev, &frame, K_NO_WAIT, can_tx_callback, &tx_queue_sem);
			if (err != 0) {
				printk("failed to enqueue CAN frame (err %d)\n", err);
			}
		}

#if DT_NODE_EXISTS(BUTTON_NODE)
		if (k_sem_take(&btn_cb_ctx.sem, K_NO_WAIT) == 0) {
			printk("button press detected, babbling stopped\n");
			return 0;
		}
#endif /* DT_NODE_EXISTS(BUTTON_NODE) */
	}
}
