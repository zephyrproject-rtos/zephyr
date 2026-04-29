/*
 * Copyright (c) 2025 Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/gap.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* Custom Hello Sensor Service UUIDs */
#define HELLO_SENSOR_SVC_UUID \
	BT_UUID_128_ENCODE(0x2f81c7b6, 0x7447, 0x46c2, 0xa4c7, 0x1ea55f2e2838)

#define HELLO_NOTIFY_CHAR_UUID \
	BT_UUID_128_ENCODE(0x29ea623d, 0x9f45, 0x44af, 0xad86, 0x3a28c75eedbd)

#define HELLO_BLINK_CHAR_UUID \
	BT_UUID_128_ENCODE(0xd2ad3650, 0x13a7, 0x4161, 0xa15a, 0x3397fa5db18e)

static struct bt_uuid_128 hello_sensor_svc_uuid = BT_UUID_INIT_128(HELLO_SENSOR_SVC_UUID);
static struct bt_uuid_128 hello_notify_uuid = BT_UUID_INIT_128(HELLO_NOTIFY_CHAR_UUID);
static struct bt_uuid_128 hello_blink_uuid = BT_UUID_INIT_128(HELLO_BLINK_CHAR_UUID);

/* Notify characteristic value */
static uint8_t hello_notify_value[7] = "Hello 0";

/* Blink characteristic value */
static uint8_t hello_blink_value = 0;

/* Notification enabled flag */
static bool notification_enabled = false;

/* Connection reference */
static struct bt_conn *default_conn;

/* GPIO devices and pins for LEDs and button */
#if DT_NODE_EXISTS(DT_ALIAS(user_led_0))
	static const struct device *led0_dev = NULL;
	static int led0_pin = -1;
#endif

#if DT_NODE_EXISTS(DT_ALIAS(user_led_1))
	static const struct device *led1_dev = NULL;
	static int led1_pin = -1;
#endif

#if DT_NODE_EXISTS(DT_ALIAS(user_btn_0))
	static const struct device *btn0_dev = NULL;
	static int btn0_pin = -1;
#endif

/* LED blink state machine */
static uint8_t led_blink_count = 0;

/* Button press handling */
#if DT_NODE_EXISTS(DT_ALIAS(user_btn_0))
	static struct gpio_callback button_cb_data;
#endif

static void led_blink_timeout(struct k_timer *timer);
K_TIMER_DEFINE(led_blink_timer, led_blink_timeout, NULL);

/**
 * @brief Callback for LED blink timer
 */
static void led_blink_timeout(struct k_timer *timer)
{
	if (led_blink_count > 0) {
#if DT_NODE_EXISTS(DT_ALIAS(user_led_0))
		if (led0_dev) {
			gpio_pin_toggle(led0_dev, led0_pin);
		}
#endif
		led_blink_count--;
		if (led_blink_count > 0) {
			k_timer_start(&led_blink_timer, K_MSEC(250), K_NO_WAIT);
		}
	}
}

/**
 * @brief Send notification to connected client
 */
static void send_notification(void)
{
	if (default_conn && notification_enabled) {
		int err;
		struct bt_gatt_notify_params params = {
			.attr = NULL,
			.data = hello_notify_value,
			.len = sizeof(hello_notify_value),
			.func = NULL,
		};

		err = bt_gatt_notify_cb(default_conn, &params);
		if (err) {
			printk("Notification send error: %d\n", err);
		} else {
			printk("Notification sent\n");
		}
	}
}

#if DT_NODE_EXISTS(DT_ALIAS(user_btn_0))
/**
 * @brief Button interrupt callback
 */
static void button_pressed(const struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	printk("Button pressed\n");
	hello_notify_value[6]++;
	send_notification();
}
#endif

/**
 * @brief Initialize GPIO LEDs and button
 */
static int gpio_init(void)
{
	int err = 0;

#if DT_NODE_EXISTS(DT_ALIAS(user_led_0))
	led0_dev = DEVICE_DT_GET(DT_ALIAS(user_led_0));
	if (!device_is_ready(led0_dev)) {
		printk("LED0 device not ready\n");
		err = -ENODEV;
	} else {
		led0_pin = DT_GPIO_PIN(DT_ALIAS(user_led_0), gpios);
		int flags = DT_GPIO_FLAGS(DT_ALIAS(user_led_0), gpios);
		if (gpio_pin_configure(led0_dev, led0_pin, GPIO_OUTPUT_ACTIVE | flags)) {
			printk("Failed to configure LED0\n");
			err = -EIO;
		} else {
			gpio_pin_set(led0_dev, led0_pin, 0);
			printk("LED0 initialized (pin %d)\n", led0_pin);
		}
	}
#endif

#if DT_NODE_EXISTS(DT_ALIAS(user_led_1))
	led1_dev = DEVICE_DT_GET(DT_ALIAS(user_led_1));
	if (!device_is_ready(led1_dev)) {
		printk("LED1 device not ready\n");
		err = -ENODEV;
	} else {
		led1_pin = DT_GPIO_PIN(DT_ALIAS(user_led_1), gpios);
		int flags = DT_GPIO_FLAGS(DT_ALIAS(user_led_1), gpios);
		if (gpio_pin_configure(led1_dev, led1_pin, GPIO_OUTPUT_ACTIVE | flags)) {
			printk("Failed to configure LED1\n");
			err = -EIO;
		} else {
			gpio_pin_set(led1_dev, led1_pin, 0);
			printk("LED1 initialized (pin %d)\n", led1_pin);
		}
	}
#endif

#if DT_NODE_EXISTS(DT_ALIAS(user_btn_0))
	btn0_dev = DEVICE_DT_GET(DT_ALIAS(user_btn_0));
	if (!device_is_ready(btn0_dev)) {
		printk("Button device not ready\n");
		err = -ENODEV;
	} else {
		btn0_pin = DT_GPIO_PIN(DT_ALIAS(user_btn_0), gpios);
		int flags = DT_GPIO_FLAGS(DT_ALIAS(user_btn_0), gpios);
		if (gpio_pin_configure(btn0_dev, btn0_pin, GPIO_INPUT | flags)) {
			printk("Failed to configure button\n");
			err = -EIO;
		} else {
			gpio_init_callback(&button_cb_data, button_pressed,
					   BIT(btn0_pin));
			gpio_add_callback(btn0_dev, &button_cb_data);
			gpio_pin_interrupt_configure(btn0_dev, btn0_pin,
						    GPIO_INT_EDGE_RISING);
			printk("Button initialized (pin %d)\n", btn0_pin);
		}
	}
#endif

	return err;
}

/* ======================== Bluetooth GATT Callbacks ======================== */

/**
 * @brief Callback when client reads Notify characteristic
 */
static ssize_t read_notify_char(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	if (offset + len > sizeof(hello_notify_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	memcpy(buf, &hello_notify_value[offset], len);
	return len;
}

/**
 * @brief Callback when client enables/disables notifications
 */
static void notify_cccd_changed(const struct bt_gatt_attr *attr,
				uint16_t value)
{
	if (value == BT_GATT_CCC_NOTIFY || value == BT_GATT_CCC_INDICATE) {
		notification_enabled = true;
		printk("Notifications enabled\n");
	} else {
		notification_enabled = false;
		printk("Notifications disabled\n");
	}
}

/**
 * @brief Callback when client reads Blink characteristic
 */
static ssize_t read_blink_char(struct bt_conn *conn,
			        const struct bt_gatt_attr *attr, void *buf,
			        uint16_t len, uint16_t offset)
{
	if (offset + len > sizeof(hello_blink_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	memcpy(buf, &hello_blink_value, len);
	return len;
}

/**
 * @brief Callback when client writes Blink characteristic
 */
static ssize_t write_blink_char(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len,
				uint16_t offset, uint8_t flags)
{
	if (offset + len > sizeof(hello_blink_value))
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	memcpy(&hello_blink_value, buf, len);
	printk("Blink characteristic written with value: %d\n", hello_blink_value);

	/* Start LED blinking */
	if (hello_blink_value > 0) {
#if DT_NODE_EXISTS(DT_ALIAS(user_led_0))
		if (led0_dev) {
			gpio_pin_set(led0_dev, led0_pin, 1);
			led_blink_count = hello_blink_value * 2; /* Each blink = on + off */
			k_timer_start(&led_blink_timer, K_MSEC(250), K_NO_WAIT);
		}
#endif
	}

	return len;
}

/* ======================== GATT Service Definition ======================== */

static struct bt_gatt_attr hello_sensor_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(&hello_sensor_svc_uuid.uuid),

	/* Notify Characteristic */
	BT_GATT_CHARACTERISTIC(&hello_notify_uuid.uuid,
			       BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_NOTIFY |
			       BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_READ,
			       read_notify_char, NULL, hello_notify_value),
	BT_GATT_CCC(notify_cccd_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	/* Blink Characteristic */
	BT_GATT_CHARACTERISTIC(&hello_blink_uuid.uuid,
			       BT_GATT_CHRC_READ |
			       BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_blink_char, write_blink_char, &hello_blink_value),
};

static struct bt_gatt_service hello_sensor_svc = BT_GATT_SERVICE(hello_sensor_attrs);

/* ======================== Connection Callbacks ======================== */

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");
	default_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	notification_enabled = false;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* ======================== Advertising ======================== */

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, HELLO_SENSOR_SVC_UUID),
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Register GATT service */
	bt_gatt_service_register(&hello_sensor_svc);

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising started\n");
}

/* ======================== Main ======================== */

int main(void)
{
	int err;

	printk("Hello Sensor Example Started\n");

	/* Initialize GPIO (LEDs and button) */
	err = gpio_init();
	if (err) {
		printk("GPIO initialization returned error: %d\n", err);
	}

	/* Initialize Bluetooth */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	/* Keep the main thread alive */
	while (1) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}

