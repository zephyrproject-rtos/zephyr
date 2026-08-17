/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/authentication/fido2/fido2.h>
#include <zephyr/authentication/fido2/fido2_transport.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/conn.h>

#if DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)
#include <zephyr/drivers/gpio.h>
#define HAS_STATUS_LED 1
#endif

#ifdef CONFIG_FIDO2_TRANSPORT_USB_HID
#include <sample_usbd.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_hid.h>
#endif

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#ifdef HAS_STATUS_LED
#define BLINK_INTERVAL_MS 150

static const struct gpio_dt_spec status_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static void blink_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	gpio_pin_toggle_dt(&status_led);
}

K_TIMER_DEFINE(blink_timer, blink_expiry, NULL);
#endif

#ifdef CONFIG_FIDO2_TRANSPORT_BLE
/*
 * The FIDO service UUID must be present in the advertising data. This sample
 * always advertises in pairing mode, so the General Discoverable flag is set.
 */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(FIDO2_BLE_SERVICE_UUID_VAL)),
};

/*
 * FIDO service data in the scan response makes pairing mode discoverable to
 * clients that do not expose the GAP discoverable flags to applications.
 */
static const uint8_t fido_service_data[] = {
	BT_UUID_16_ENCODE(FIDO2_BLE_SERVICE_UUID_VAL),
	FIDO2_BLE_SERVICE_DATA_PAIRING_MODE,
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_SVC_DATA16, fido_service_data, sizeof(fido_service_data)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1U),
};

static int start_advertising(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err != 0) {
		LOG_ERR("Failed to start FIDO2 BLE advertising: %d", err);
		return err;
	}

	LOG_INF("FIDO2 BLE advertising started");
	return 0;
}

static void connection_recycled(void)
{
	int err;

	(void)start_advertising();
}

BT_CONN_CB_DEFINE(sample_conn_callbacks) = {
	.recycled = connection_recycled,
};
#endif

#ifdef CONFIG_FIDO2_TRANSPORT_USB_HID
struct usbd_context *sample_usbd;

static void msg_cb(struct usbd_context *const usbd_ctx, const struct usbd_msg *const msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (msg->type == USBD_MSG_CONFIGURATION) {
		LOG_INF("\tConfiguration value %d", msg->status);
	}

	if (usbd_can_detect_vbus(usbd_ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(usbd_ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(usbd_ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}
}
#endif

static const char *fido2_state_to_str(enum fido2_runtime_state state)
{
	switch (state) {
	case FIDO2_RUNTIME_STATE_STOPPED:
		return "stopped";
	case FIDO2_RUNTIME_STATE_IDLE:
		return "idle";
	case FIDO2_RUNTIME_STATE_WAITING_USER_PRESENCE:
		return "waiting_user_presence";
	case FIDO2_RUNTIME_STATE_PROCESSING:
		return "processing";
	default:
		return "unknown";
	}
}

static void fido2_state_changed(enum fido2_runtime_state state, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_INF("FIDO2 runtime state: %s", fido2_state_to_str(state));

#ifdef HAS_STATUS_LED
	switch (state) {
	case FIDO2_RUNTIME_STATE_WAITING_USER_PRESENCE:
		k_timer_start(&blink_timer, K_NO_WAIT, K_MSEC(BLINK_INTERVAL_MS));
		break;
	case FIDO2_RUNTIME_STATE_PROCESSING:
		k_timer_stop(&blink_timer);
		gpio_pin_set_dt(&status_led, 1);
		break;
	default:
		k_timer_stop(&blink_timer);
		gpio_pin_set_dt(&status_led, 0);
		break;
	}
#endif
}

int main(void)
{
	int ret;

	ret = fido2_init();
	if (ret) {
		LOG_ERR("FIDO2 init failed: %d", ret);
		return ret;
	}

#ifdef HAS_STATUS_LED
	if (!gpio_is_ready_dt(&status_led)) {
		LOG_WRN("led0 not ready, LED indication disabled");
	} else {
		ret = gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_WRN("Failed to configure led0: %d", ret);
		}
	}
#endif

	ret = fido2_set_state_callback(fido2_state_changed, NULL);
	if (ret) {
		LOG_ERR("Failed to register state callback: %d", ret);
		return ret;
	}

#ifdef CONFIG_FIDO2_TRANSPORT_USB_HID
	sample_usbd = sample_usbd_init_device(msg_cb);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(sample_usbd)) {
		ret = usbd_enable(sample_usbd);
		if (ret) {
			LOG_ERR("Failed to enable device support");
			return ret;
		}
	}
#endif

	ret = fido2_start();
	if (ret != 0) {
		LOG_ERR("fido2_start failed: %d", ret);
		return 0;
	}

	LOG_INF("FIDO2 started");

#ifdef CONFIG_FIDO2_TRANSPORT_BLE
	ret = bt_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Bluetooth init failed: %d", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		ret = settings_load();
		if (ret != 0) {
			LOG_ERR("Failed to load Bluetooth settings: %d", ret);
			return ret;
		}
	}

	ret = start_advertising();
	if (ret != 0) {
		return ret;
	}
#endif

	LOG_INF("FIDO2 authenticator ready");
	k_sleep(K_FOREVER);

	return 0;
}
