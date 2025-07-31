/*
 * Copyright (c) 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb_otg.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(usb_otg_sample, LOG_LEVEL_INF);

static const struct device *otg_dev;
static enum usb_otg_role current_role = USB_OTG_ROLE_NONE;

/* USB device mode CDC ACM variables */
static const struct device *cdc_dev;
static struct cdc_acm_cfg cdc_config;

/* USB host mode callback */
static void usb_host_status_cb(struct usb_device *udev,
			       const struct usbh_message *const msg)
{
	if (msg->type == USBH_MSG_DEVICE_ADDED) {
		LOG_INF("USB device connected in host mode");
		LOG_INF("  VID: 0x%04x, PID: 0x%04x",
			le16_to_cpu(udev->descriptor.idVendor),
			le16_to_cpu(udev->descriptor.idProduct));
	} else if (msg->type == USBH_MSG_DEVICE_REMOVED) {
		LOG_INF("USB device disconnected");
	}
}

/* USB device mode CDC ACM callback */
static void cdc_acm_handler(uint8_t ep, enum usb_dc_ep_cb_status_code status)
{
	if (status == USB_DC_EP_DATA_IN) {
		LOG_DBG("CDC ACM data sent");
	}
}

/* USB OTG event callback */
static void otg_event_callback(const struct device *dev,
			      enum usb_otg_event event,
			      enum usb_otg_role role)
{
	int ret;

	LOG_INF("USB OTG event: %d, role: %s", event,
		role == USB_OTG_ROLE_HOST ? "HOST" :
		role == USB_OTG_ROLE_DEVICE ? "DEVICE" : "NONE");

	if (role == current_role) {
		return;
	}

	/* Handle role transitions */
	if (current_role == USB_OTG_ROLE_HOST) {
		/* Disable USB host */
		usbh_disable();
		LOG_INF("USB host mode disabled");
	} else if (current_role == USB_OTG_ROLE_DEVICE) {
		/* Disable USB device */
		usb_disable();
		LOG_INF("USB device mode disabled");
	}

	current_role = role;

	/* Enable new role */
	if (role == USB_OTG_ROLE_HOST) {
		/* Initialize and enable USB host */
		ret = usbh_init();
		if (ret == 0) {
			ret = usbh_register_callback(usb_host_status_cb);
			if (ret == 0) {
				ret = usbh_enable();
				if (ret == 0) {
					LOG_INF("USB host mode enabled");
				}
			}
		}
		if (ret != 0) {
			LOG_ERR("Failed to enable USB host mode: %d", ret);
		}
	} else if (role == USB_OTG_ROLE_DEVICE) {
		/* Initialize and enable USB device */
		ret = usb_enable(NULL);
		if (ret == 0) {
			LOG_INF("USB device mode enabled");
		} else {
			LOG_ERR("Failed to enable USB device mode: %d", ret);
		}
	}
}

int main(void)
{
	int ret;

	LOG_INF("USB OTG sample application");

	/* Get OTG device */
	otg_dev = DEVICE_DT_GET_ONE(nxp_lpc_otg);
	if (!device_is_ready(otg_dev)) {
		LOG_ERR("USB OTG device not ready");
		return -ENODEV;
	}

	/* Initialize USB device CDC ACM */
	cdc_dev = device_get_binding("CDC_ACM_0");
	if (!cdc_dev) {
		LOG_ERR("CDC ACM device not found");
		return -ENODEV;
	}

	/* Register USB device callbacks */
	cdc_config.cb = cdc_acm_handler;
	ret = cdc_acm_set_config(cdc_dev, &cdc_config);
	if (ret < 0) {
		LOG_ERR("Failed to configure CDC ACM");
		return ret;
	}

	/* Initialize OTG controller */
	ret = usb_otg_init(otg_dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize USB OTG: %d", ret);
		return ret;
	}

	/* Register OTG event callback */
	ret = usb_otg_register_callback(otg_dev, otg_event_callback);
	if (ret < 0) {
		LOG_ERR("Failed to register OTG callback: %d", ret);
		return ret;
	}

	/* Enable OTG controller */
	ret = usb_otg_enable(otg_dev);
	if (ret < 0) {
		LOG_ERR("Failed to enable USB OTG: %d", ret);
		return ret;
	}

	LOG_INF("USB OTG initialized - connect USB cable with ID pin");
	LOG_INF("ID pin LOW = Host mode, ID pin HIGH = Device mode");

	/* Main loop */
	while (1) {
		if (current_role == USB_OTG_ROLE_DEVICE && cdc_dev) {
			/* In device mode, send periodic message */
			static const char msg[] = "Hello from USB OTG device mode!\r\n";
			cdc_acm_write(cdc_dev, (const uint8_t *)msg, sizeof(msg) - 1);
		}
		k_sleep(K_SECONDS(5));
	}

	return 0;
}