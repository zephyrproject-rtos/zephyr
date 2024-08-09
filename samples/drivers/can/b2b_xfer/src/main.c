/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/can.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#if defined(CONFIG_SAMPLE_CAN_B2B_XFER_ROLE_SENDER)
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
static struct gpio_callback button_cb_data;
#endif /* CONFIG_SAMPLE_CAN_B2B_XFER_ROLE */

#define CANBUS_NODE DT_CHOSEN(zephyr_canbus)
#if !DT_NODE_HAS_STATUS(CANBUS_NODE, okay)
#error "Unsupported board: zephyr_canbus devicetree node is not defined"
#endif
const struct device *const can_dev = DEVICE_DT_GET(CANBUS_NODE);

CAN_MSGQ_DEFINE(can_dev_msgq, 2U);

#if defined(CONFIG_SAMPLE_CAN_B2B_XFER_ROLE_SENDER)
k_tid_t tid_main;
uint32_t txIdentifier = CONFIG_SAMPLE_CAN_B2B_XFER_SENDER_CAN_ID;
uint32_t rxIdentifier = CONFIG_SAMPLE_CAN_B2B_XFER_REPLIER_CAN_ID;
#elif defined(CONFIG_SAMPLE_CAN_B2B_XFER_ROLE_REPLIER)
uint32_t txIdentifier = CONFIG_SAMPLE_CAN_B2B_XFER_REPLIER_CAN_ID;
uint32_t rxIdentifier = CONFIG_SAMPLE_CAN_B2B_XFER_SENDER_CAN_ID;
#endif /* CONFIG_SAMPLE_CAN_B2B_XFER_ROLE */

#if defined(CONFIG_SAMPLE_CAN_B2B_XFER_ROLE_SENDER)
void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	k_wakeup(tid_main);
}
#endif /* CONFIG_SAMPLE_CAN_B2B_XFER_ROLE */

int main(void)
{
	int ret;

#if defined(CONFIG_SAMPLE_CAN_B2B_XFER_ROLE_SENDER)
	tid_main = k_current_get();

	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return ret;
	}
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, button.port->name, button.pin);
		return ret;
	}
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	printk("Set up button at %s pin %d\n", button.port->name, button.pin);
#endif /* CONFIG_SAMPLE_CAN_B2B_XFER_ROLE */

	if (!device_is_ready(can_dev)) {
		printk("Error: CAN device %s not ready.\n", can_dev->name);
		return -ENODEV;
	}

#ifdef CONFIG_CAN_FD_MODE
	ret = can_set_mode(can_dev, CAN_MODE_FD);
	if (ret != 0) {
		printk("Error setting CAN FD mode (err %d)", ret);
		return -ret;
	}
#endif /* CONFIG_CAN_FD_MODE */
	ret = can_start(can_dev);
	if (ret != 0) {
		printk("Error starting CAN controller (err %d)", ret);
		return ret;
	}

	struct can_filter filter = {
		.flags = 0,
		.id = rxIdentifier,
		.mask = CAN_STD_ID_MASK
	};
	int filter_id = can_add_rx_filter_msgq(can_dev, &can_dev_msgq, &filter);
	if (filter_id < 0) {
		printk("rx filter error [%d]", filter_id);
		return -ENODEV;
	}
	printk("Add rx filter %d with can_id: 0x%3.3x", filter_id, rxIdentifier);

	struct can_frame frame = {
#ifdef CONFIG_CAN_FD_MODE
		.flags = CAN_FRAME_FDF | CAN_FRAME_BRS,
#else
		.flags = 0U,
#endif /* CONFIG_CAN_FD_MODE */
		.id = txIdentifier,
		.data_32 = { 0 },
	};
	printk("Set tx frame with can_id: 0x%3.3x", txIdentifier);

	printk("\n\n**** CAN b2b xfer demo ****\n\n"
#if defined(CONFIG_SAMPLE_CAN_B2B_XFER_ROLE_SENDER)
		   "This node is sender, press button to send CAN frame\n"
#elif defined(CONFIG_SAMPLE_CAN_B2B_XFER_ROLE_REPLIER)
		   "This node is replier, wait for CAN frame from sender\n"
#endif /* CONFIG_SAMPLE_CAN_B2B_XFER_ROLE */
	);

	while (true) {
#if defined(CONFIG_SAMPLE_CAN_B2B_XFER_ROLE_SENDER)
#ifdef CONFIG_CAN_FD_MODE
		frame.flags = CAN_FRAME_FDF | CAN_FRAME_BRS;
#else
		frame.flags = 0U;
#endif // ! CONFIG_CAN_FD_MODE
		frame.id = txIdentifier;
		frame.dlc = 4U;
		printk("Press button to send...\n");
		k_thread_suspend(tid_main);
		printk("Tx id: 0x%3.3x data: 0x%8.8x\n", frame.id, frame.data[0]);
		ret = can_send(can_dev, &frame, K_FOREVER, NULL, NULL);
		k_msgq_get(&can_dev_msgq, &frame, K_FOREVER);
		printk("Rx id: 0x%3.3x data: 0x%8.8x\n", frame.id, frame.data[0]);
#elif defined(CONFIG_SAMPLE_CAN_B2B_XFER_ROLE_REPLIER)
		printk("Wait for sender node...\n");
		k_msgq_get(&can_dev_msgq, &frame, K_FOREVER);
		printk("Rx id:0x%3.3x data: 0x%8.8x\n", frame.id, frame.data[0]);
#ifdef CONFIG_CAN_FD_MODE
		frame.flags = CAN_FRAME_FDF | CAN_FRAME_BRS;
#else
		frame.flags = 0U;
#endif // ! CONFIG_CAN_FD_MODE
		frame.id = txIdentifier;
		frame.dlc = 4U;
		frame.data_32[0]++;
		printk("Tx id: 0x%3.3x data: 0x%8.8x\n", frame.id, frame.data[0]);
		ret = can_send(can_dev, &frame, K_FOREVER, NULL, NULL);
#endif /* CONFIG_SAMPLE_CAN_B2B_XFER_ROLE */
	}

	return 0;
}
