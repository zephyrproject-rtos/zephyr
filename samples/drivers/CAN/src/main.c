/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <misc/printk.h>
#include <device.h>
#include <can.h>
#include <gpio.h>

#define TX_THREAD_STACK_SIZE 512
#define LED_THREAD_STACK_SIZE 512
#define RX_STR_THREAD_STACK_SIZE 512
#define TX_THREAD_PRIORITY 2
#define LED_MSG_ID (0x10)
#define BUTTON_MSG_ID (0x01)
#define STR_MSG_ID (0x12345)

#define SET_LED 0
#define RESET_LED 1


#define NUM_LEDS_STR STRINGIFY(NUM_LEDS)

K_THREAD_STACK_DEFINE(tx_thread_stack, TX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(led_thread_stack, LED_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(rx_str_thread_stack, RX_STR_THREAD_STACK_SIZE);
struct k_thread tx_thread_data;
struct k_thread led_thread_data;
struct k_thread rx_str_thread_data;
struct k_sem tx_sem;
static struct gpio_callback gpio_cb;
CAN_DEFINE_MSGQ(led_msgq, 2);
CAN_DEFINE_MSGQ(str_msgq, 5);

void tx_irq_callback(u32_t error_flags)
{
	if (error_flags) {
		printk("Callback! error-code: %d\n", error_flags);
	}
}

void button_callback(struct device *port,
		     struct gpio_callback *cb, u32_t pins)
{
	k_sem_give(&tx_sem);
}

void send_string(char *string, struct device *can_dev)
{
	struct can_msg msg;
	int str_len;

	msg.ext_id = STR_MSG_ID;
	msg.id_type = CAN_EXTENDED_IDENTIFIER;
	msg.dlc = 0;
	msg.rtr = CAN_DATAFRAME;

	for (str_len = strlen(string); str_len; ) {
		msg.dlc = str_len >= 8 ? 8 : str_len;
		str_len -= msg.dlc;
		memcpy(msg.data, string, msg.dlc);
		string += msg.dlc;
		can_send(can_dev, &msg, 10, tx_irq_callback);
	}
}

void tx_thread(void *can_dev_param, void *unused2, void *unused3)
{
	u8_t toggle = SET_LED;
	u16_t button_press_cnt = 0;
	struct can_msg msg;
	struct can_msg msg_button_cnt;
	struct device *can_dev = can_dev_param;

	msg.std_id = LED_MSG_ID;
	msg.id_type = CAN_STANDARD_IDENTIFIER;
	msg.dlc = 1;
	msg.rtr = CAN_DATAFRAME;
	msg.data[0] = 0;

	msg_button_cnt.std_id = BUTTON_MSG_ID;
	msg_button_cnt.id_type = CAN_STANDARD_IDENTIFIER;
	msg_button_cnt.dlc = 2;
	msg_button_cnt.rtr = CAN_DATAFRAME;
	msg_button_cnt.data[0] = 0;
	msg_button_cnt.data[1] = 0;

	printk("TX thread is running.\n");
	while (1) {
		k_sem_take(&tx_sem, K_FOREVER);
		button_press_cnt++;
		toggle = (toggle == SET_LED) ? RESET_LED : SET_LED;
		printk("Button pressed! Send message %u\n", toggle);
		msg.data[0] = toggle;
		msg_button_cnt.data[0] = button_press_cnt & 0xFF;
		msg_button_cnt.data[1] = (button_press_cnt >> 8) & 0xFF;
		can_send(can_dev, &msg, 10, tx_irq_callback);
		can_send(can_dev, &msg_button_cnt, 10, NULL);
		if (toggle == SET_LED) {
			send_string("String sent over CAN\n", can_dev);
		}
	}
}

void rx_str_thread(void *msgq, void *can_dev_param, void *unused)
{
	struct can_msg msg;
	const struct can_filter filter = {
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.ext_id = STR_MSG_ID,
		.rtr_mask = 1,
		.ext_id_mask = CAN_EXT_ID_MASK
	};
	struct device *can_dev = can_dev_param;

	can_attach_msgq(can_dev, msgq, &filter);

	while (1) {
		k_msgq_get((struct k_msgq *)msgq, &msg, K_FOREVER);
		for (int i = 0; i < msg.dlc; i++)
			printk("%c", msg.data[i]);
	}
}

void led_thread(void *msgq, void *can_dev_param, void *gpio_dev_param)
{
	const struct can_filter filter = {
		.id_type = CAN_STANDARD_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.std_id = LED_MSG_ID,
		.rtr_mask = 1,
		.std_id_mask = CAN_STD_ID_MASK
	};
	struct device *can_dev = can_dev_param;
	struct device *gpio_dev = gpio_dev_param;
	struct can_msg msg;
	int ret;
	int filter_id;

	ret = gpio_pin_configure(gpio_dev, CONFIG_PIN_LED_1, GPIO_DIR_OUT);
	gpio_pin_write(gpio_dev, CONFIG_PIN_LED_1, 0);

	if (ret) {
		printk("ERROR configure pins\n");
		return;
	}

	filter_id = can_attach_msgq(can_dev, msgq, &filter);
	printk("filter id: %d\n", filter_id);

	while (1) {
		k_msgq_get((struct k_msgq *)msgq, &msg, K_FOREVER);

		if (msg.dlc != 1) {
			continue;
		}

		switch (msg.data[0]) {
		case SET_LED:
			gpio_pin_write(gpio_dev, CONFIG_PIN_LED_1, 1);

			break;
		case RESET_LED:
			gpio_pin_write(gpio_dev, CONFIG_PIN_LED_1, 0);
			break;
		}
	}
}

void rx_button_isr(struct can_msg *msg)
{
	u16_t cnt = msg->data[0] | (msg->data[1] << 8);

	printk("Button pressed %d times\n", cnt);
}

void main(void)
{
	const struct can_filter filter = {
		.id_type = CAN_STANDARD_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.std_id = BUTTON_MSG_ID,
		.rtr_mask = 1,
		.std_id_mask = CAN_STD_ID_MASK
	};
	struct device *can_dev, *led_gpio_dev, *button_gpio_dev;
	int ret;

	can_dev = device_get_binding(CONFIG_CAN_DEV);
	if (!can_dev) {
		printk("CAN: Device driver not found.\n");
		return;
	}

#ifdef CONFIG_LOOPBACK_MODE
	can_configure(can_dev, CAN_LOOPBACK_MODE, 250000);
#endif

	led_gpio_dev = device_get_binding(CONFIG_GPIO_LED_DEV);
	if (!led_gpio_dev) {
		printk("LED: Device driver not found.\n");
		return;
	}

	k_sem_init(&tx_sem, 0, INT_MAX);

	button_gpio_dev = device_get_binding(CONFIG_GPIO_BUTTON_DEV);
	if (!button_gpio_dev) {
		printk("Button: Device driver not found.\n");
		return;
	}

	ret = gpio_pin_configure(button_gpio_dev, CONFIG_PIN_USER_BUTTON,
				    (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
				     GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE));
	if (ret) {
		printk("Error configuring  button pin\n");
	}

	gpio_init_callback(&gpio_cb, button_callback,
			   BIT(CONFIG_PIN_USER_BUTTON));

	ret = gpio_add_callback(button_gpio_dev, &gpio_cb);
	if (ret) {
		printk("Cannot setup callback!\n");
	}

	ret = gpio_pin_enable_callback(button_gpio_dev, CONFIG_PIN_USER_BUTTON);
	if (ret) {
		printk("Error enabling callback!\n");
	}

	can_attach_isr(can_dev, rx_button_isr, &filter);

	k_tid_t tx_tid = k_thread_create(&tx_thread_data, tx_thread_stack,
					 K_THREAD_STACK_SIZEOF(tx_thread_stack),
					 tx_thread,
					 can_dev, NULL, NULL,
					 TX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!tx_tid) {
		printk("ERROR spawning tx_thread\n");
	}

	k_tid_t led_tid = k_thread_create(&led_thread_data, led_thread_stack,
					  K_THREAD_STACK_SIZEOF(led_thread_stack),
					  led_thread,
					  &led_msgq, can_dev, led_gpio_dev,
					  TX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!led_tid) {
		printk("ERROR spawning led_thread\n");
	}

	k_tid_t str_tid = k_thread_create(&rx_str_thread_data,
					  rx_str_thread_stack,
					  K_THREAD_STACK_SIZEOF(rx_str_thread_stack),
					  rx_str_thread,
					  &str_msgq, can_dev, NULL,
					  TX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!str_tid) {
		printk("ERROR spawning str_thread\n");
	}

	printk("Finished init. waiting for Interrupts\n");
}
