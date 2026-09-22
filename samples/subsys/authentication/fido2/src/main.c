/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/authentication/fido2/fido2.h>

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
	if (ret) {
		LOG_ERR("FIDO2 start failed: %d", ret);
		return ret;
	}

	LOG_INF("FIDO2 authenticator ready");
	k_sleep(K_FOREVER);

	return 0;
}
