/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usbd_hid_internal.h"

#include <stdint.h>
#include <assert.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/usb/class/usbd_hid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hid_api, CONFIG_USBD_HID_LOG_LEVEL);

int hid_device_submit_report(const struct device *dev,
			     const uint16_t size, const uint8_t *const report)
{
	const struct hid_device_driver_api *api = dev->api;

	return api->submit_report(dev, size, report);
}

int hid_device_register(const struct device *dev,
			const uint8_t *const rdesc, const uint16_t rsize,
			const struct hid_device_ops *const ops)
{
	const struct hid_device_driver_api *api = dev->api;

	return api->dev_register(dev, rdesc, rsize, ops);
}

/* Legacy HID API wrapper below */

struct legacy_wrapper {
	const struct device *dev;
	const struct hid_ops *legacy_ops;
	struct hid_device_ops *ops;
};

static struct hid_device_ops wrapper_ops;

#define DT_DRV_COMPAT zephyr_hid_device

#define USBD_HID_WRAPPER_DEFINE(n)						\
	{									\
		.dev = DEVICE_DT_GET(DT_DRV_INST(n)),				\
		.ops = &wrapper_ops,						\
	},

static struct legacy_wrapper wrappers[] = {
	DT_INST_FOREACH_STATUS_OKAY(USBD_HID_WRAPPER_DEFINE)
};

static const struct hid_ops *get_legacy_ops(const struct device *dev)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(wrappers); i++) {
		if (wrappers[i].dev == dev) {
			return wrappers[i].legacy_ops;
		}
	}

	return NULL;
}

int wrapper_get_report(const struct device *dev,
		       const uint8_t type, const uint8_t id,
		       const uint16_t len, uint8_t *const buf)
{
	const struct hid_ops *legacy_ops = get_legacy_ops(dev);
	struct usb_setup_packet setup = {
		.bmRequestType = 0,
		.bRequest = 0,
		.wValue = (type << 8) | id,
		.wIndex = 0,
		.wLength = len,
	};
	uint8_t *d = buf;
	int l = len;

	if (legacy_ops != NULL && legacy_ops->get_report != NULL) {
		return legacy_ops->get_report(dev, &setup, &l, &d);
	}

	return -ENOTSUP;
}

int wrapper_set_report(const struct device *dev,
		       const uint8_t type, const uint8_t id,
		       const uint16_t len, const uint8_t *const buf)
{
	const struct hid_ops *legacy_ops = get_legacy_ops(dev);
	struct usb_setup_packet setup = {
		.bmRequestType = 0,
		.bRequest = 0,
		.wValue = (type << 8) | id,
		.wIndex = 0,
		.wLength = len,
	};
	uint8_t *d = (void *)buf;
	int l = len;

	if (legacy_ops != NULL && legacy_ops->set_report != NULL) {
		return legacy_ops->set_report(dev, &setup, &l, &d);
	}

	return -ENOTSUP;
}

void wrapper_set_idle(const struct device *dev,
		      const uint8_t id, const uint32_t duration)
{
	if (id != 0U) {
		LOG_ERR("Set Idle for %s ID %u duration %u cannot be propagated",
			dev->name, id, duration);
	}
}

void wrapper_set_protocol(const struct device *dev, const uint8_t proto)
{
	const struct hid_ops *legacy_ops = get_legacy_ops(dev);

	if (legacy_ops != NULL && legacy_ops->protocol_change != NULL) {
		legacy_ops->protocol_change(dev, proto);
	}
}

void wrapper_input_report_done(const struct device *dev,
			       const uint8_t *const report)
{
	ARG_UNUSED(report);

	const struct hid_ops *legacy_ops = get_legacy_ops(dev);

	if (legacy_ops != NULL && legacy_ops->int_in_ready != NULL) {
		legacy_ops->int_in_ready(dev);
	}
}

void wrapper_output_report(const struct device *dev,
			   const uint16_t len, const uint8_t *const buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(len);
	ARG_UNUSED(buf);

	__ASSERT(false, "Output report callback is not supported");
}

static struct hid_device_ops wrapper_ops = {
	.get_report = wrapper_get_report,
	.set_report = wrapper_set_report,
	.set_idle = wrapper_set_idle,
	.set_protocol = wrapper_set_protocol,
	.input_report_done = wrapper_input_report_done,
	.output_report = wrapper_output_report,
};

int hid_int_ep_write(const struct device *dev,
		     const uint8_t *data, uint32_t data_len, uint32_t *bytes_ret)
{
	int ret;

	ret = hid_device_submit_report(dev, data_len, data);
	if (bytes_ret != NULL) {
		*bytes_ret = ret == 0 ? data_len : 0;
	}

	return ret;
}

int hid_int_ep_read(const struct device *dev,
		    uint8_t *data, uint32_t max_data_len, uint32_t *ret_bytes)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	ARG_UNUSED(max_data_len);
	ARG_UNUSED(ret_bytes);

	LOG_ERR("Not supported");

	return -ENOTSUP;
}

int usb_hid_set_proto_code(const struct device *dev, uint8_t proto_code)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(proto_code);

	LOG_WRN("Protocol code is set using DT property protocol-code");

	return 0;
}

int usb_hid_init(const struct device *dev)
{
	LOG_DBG("It does nothing for dev %s", dev->name);

	return 0;
}

void usb_hid_register_device(const struct device *dev,
			     const uint8_t *desc, size_t size,
			     const struct hid_ops *ops)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(wrappers); i++) {
		if (wrappers[i].dev == dev) {
			wrappers[i].legacy_ops = ops;
			if (hid_device_register(dev, desc, size, wrappers[i].ops)) {
				LOG_ERR("Failed to register HID device");
			}
		}
	}

}
