/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include "usbh_internal.h"
#include <zephyr/sys/iterable_sections.h>
#include "usbh_ch9.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhs, CONFIG_USBH_LOG_LEVEL);

/* data stage number in a control transfer */
#define DATA_STAGE	       (2u)
/* device default address used during enumeration */
#define DEVICE_DEFAULT_ADDRESS (0u)
/* additional timeout to sync driver and core layers in case of transfers timeout */
#define USBH_XFER_SEM_TIMEOUT  (CONFIG_USBH_XFER_SEM_TIMEOUT + 300u)
/* delay to check device connection status in case of false connections */
#define USBH_DEV_CONN_DELAY    (100u)

static K_KERNEL_STACK_DEFINE(usbh_enum_stack, CONFIG_USBH_ENUM_STACK_SIZE);
static struct k_thread usbh_enum_thread_data;

static K_KERNEL_STACK_DEFINE(usbh_stack, CONFIG_USBH_STACK_SIZE);
static struct k_thread usbh_thread_data;

/* TODO */
static struct usbh_class_data *class_data;

K_MSGQ_DEFINE(usbh_msgq, sizeof(struct uhc_event), CONFIG_USBH_MAX_UHC_MSG, sizeof(uint32_t));

static int usbh_event_carrier(const struct device *dev, const struct uhc_event *const event)
{
	return k_msgq_put(&usbh_msgq, event, K_NO_WAIT);
}

static int usbh_populate_descriptor(const struct device *dev, uint8_t *buf, uint16_t len)
{
	struct uhc_data *data = dev->data;
	uint8_t ep_index = 0;

	if (!data) {
		return -ENODATA;
	}

	if (data->enum_meta_data.req_type == REQ_GET_DEV_DESCRIPTOR) {
		if (len == sizeof(struct usb_device_descriptor)) {
			LOG_DBG("copying device descriptor");
			memcpy((uint8_t *)&data->dev_descriptor, buf, len);

			const struct usb_device_descriptor *const dev_desc = &data->dev_descriptor;

			LOG_INF("device descriptor:\n");
			LOG_INF(
			"\tbLength = 0x%X\n"
			"\tbDescriptorType = 0x%X\n"
			"\tbcdUSB = 0x%X\n"
			"\tbDeviceClass = 0x%X\n"
			"\tbDeviceSubClass = 0x%X\n"
			"\tbDeviceProtocol = 0x%X\n"
			"\tbMaxPacketSize = 0x%X\n"
			"\tidVendor = 0x%X\n"
			"\tidProduct = 0x%X\n"
			"\tbcdDevice = 0x%X\n"
			"\tiManufacturer = 0x%X\n"
			"\tiProduct = 0x%X\n"
			"\tiSerialNumber = 0x%X\n"
			"\tbNumConfigurations = 0x%X\n",
			dev_desc->bLength,
			dev_desc->bDescriptorType,
			dev_desc->bcdUSB,
			dev_desc->bDeviceClass,
			dev_desc->bDeviceSubClass,
			dev_desc->bDeviceProtocol,
			dev_desc->bMaxPacketSize0,
			dev_desc->idVendor,
			dev_desc->idProduct,
			dev_desc->bcdDevice,
			dev_desc->iManufacturer,
			dev_desc->iProduct,
			dev_desc->iSerialNumber,
			dev_desc->bNumConfigurations);
		}
	} else if (data->enum_meta_data.req_type == REQ_GET_CFG_DESCRIPTOR) {
		LOG_DBG("copying config, if and ep descriptors");

		memcpy((uint8_t *)&data->cfg_descriptor, buf, sizeof(struct usb_cfg_descriptor));
		buf += sizeof(struct usb_cfg_descriptor);

		memcpy((uint8_t *)&data->if_descriptor, buf, sizeof(struct usb_if_descriptor));
		buf += sizeof(struct usb_if_descriptor);

		const struct usb_if_descriptor *const if_desc = &data->if_descriptor;

		LOG_INF("interface descriptor:\n");
		LOG_INF(
				"\tbLength = 0x%X\n"
				"\tbDescriptorType = 0x%X\n"
				"\tbInterfaceNumber = 0x%X\n"
				"\tbAlternateSetting = 0x%X\n"
				"\tbNumEndpoints = 0x%X\n"
				"\tbInterfaceClass = 0x%X\n"
				"\tbInterfaceSubClass = 0x%X\n"
				"\tbInterfaceProtocol = 0x%X\n"
				"\tInterface = 0x%X\n",
				if_desc->bLength,
				if_desc->bDescriptorType,
				if_desc->bInterfaceNumber,
				if_desc->bAlternateSetting,
				if_desc->bNumEndpoints,
				if_desc->bInterfaceClass,
				if_desc->bInterfaceSubClass,
				if_desc->bInterfaceProtocol,
				if_desc->iInterface);

		for (ep_index = 0; ep_index < data->if_descriptor.bNumEndpoints; ep_index++) {
			memcpy((uint8_t *)&data->ep_descriptor[ep_index], buf,
			       sizeof(struct usb_ep_descriptor));
			buf += sizeof(struct usb_ep_descriptor);
		}
	}

	return 0;
}

static int event_ep_request(struct usbh_contex *const ctx, struct uhc_event *const event)
{
	struct uhc_transfer *xfer = event->xfer;
	const struct device *dev = ctx->dev;
	struct uhc_data *data = dev->data;
	uint8_t xfer_stage = 0;

	if (class_data && class_data->request) {
		return class_data->request(ctx, event->xfer, event->status);
	}

	while (!k_fifo_is_empty(&xfer->done)) {
		xfer_stage++;
		struct net_buf *buf;

		buf = net_buf_get(&xfer->done, K_NO_WAIT);

		if (buf) {
			LOG_HEXDUMP_INF(buf->data, buf->len, "buf");
			if (atomic_test_bit(&data->enum_meta_data.state, ENUM_IN_PROGRESS_STATUS)
				&& xfer_stage == DATA_STAGE) {
				if (!event->status) {
					usbh_populate_descriptor(dev, buf->data, buf->size);
				}
			}
			uhc_xfer_buf_free(dev, buf);
		}
	}

	if (atomic_test_bit(&data->enum_meta_data.state, ENUM_IN_PROGRESS_STATUS)) {
		if (event->status < 0) {
			LOG_ERR("transfer failed");
		} else {
			k_sem_give(&data->enum_meta_data.enum_sem);
		}
	}

	return uhc_xfer_free(dev, xfer);
}

static void usbh_register_dev(void)
{
	STRUCT_SECTION_FOREACH(register_usbh, ndev)
	{
		ndev->pre_init();
	}
}

static void usbh_register_class(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	STRUCT_SECTION_FOREACH(usbh_class_data, cdata)
	{
		LOG_DBG("class data %p", cdata);

		/* check for a class match for the devices' interface */
		if ((cdata->code.dclass == data->if_descriptor.bInterfaceClass) &&
		    (cdata->code.sub == data->if_descriptor.bInterfaceSubClass) &&
		    (cdata->code.proto == data->if_descriptor.bInterfaceProtocol)) {
			LOG_DBG("class found");
			class_data = cdata;
			break;
		}
	}
}

/* Process enumeration request response */
static inline void process_enum_req_resp(struct uhc_data *data, int err, uint32_t *retry_desc,
		enum uhc_enumeration_req_type next_req, bool *enum_complete)
{
	int sem_ret = 0;

	if (!err) {
		sem_ret = k_sem_take(&data->enum_meta_data.enum_sem,
			K_MSEC(USBH_XFER_SEM_TIMEOUT));
		if (!sem_ret) {
			*retry_desc = 0;
			data->enum_meta_data.req_type = next_req;
		} else {
			*retry_desc = *retry_desc + 1;
		}
	} else {
		/* stop execution in case of error */
		*enum_complete = true;
	}
}

/* Thread to handle enumeration */
static int usbh_enum_thread(struct usbh_contex *const ctx)
{
	const struct device *dev = ctx->dev;
	struct uhc_data *data = dev->data;
	int err = 0;
	uint32_t retry_desc = 0;
	bool enum_complete = false;
	bool enum_success = false;

	atomic_set_bit(&data->enum_meta_data.state, ENUM_IN_PROGRESS_STATUS);
	data->enum_meta_data.req_type = REQ_GET_DEV_DESCRIPTOR;
	k_sem_init(&data->enum_meta_data.enum_sem, 0, 1);

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (!uhc_is_enabled(ctx->dev)) {
		err = usbh_enable(ctx);
	}
	k_mutex_unlock(&ctx->mutex);

	if (err) {
		return err;
	}

	printk("Enumeration started\n");
	while (atomic_test_bit(&data->status, UHC_STATUS_DEV_CONN) &&
		(retry_desc <= CONFIG_USBH_ENUM_MAX_RETRY) && !enum_complete) {
		switch (data->enum_meta_data.req_type) {
		case REQ_GET_DEV_DESCRIPTOR:
			LOG_DBG("req dev desc");
			err = usbh_req_desc_dev(dev, DEVICE_DEFAULT_ADDRESS);
			process_enum_req_resp(data, err, &retry_desc, REQ_SET_DEV_ADDRESS,
				&enum_complete);
			break;
		case REQ_SET_DEV_ADDRESS:
			LOG_DBG("req set addr");
			err = usbh_req_set_address(dev, 0, 1);
			process_enum_req_resp(data, err, &retry_desc, REQ_GET_CFG_DESCRIPTOR,
				&enum_complete);
			break;
		case REQ_GET_CFG_DESCRIPTOR:
			LOG_DBG("req dev cfg");
			err = usbh_req_desc_cfg(dev, 1, 0, 32);
			process_enum_req_resp(data, err, &retry_desc, REQ_SET_DEV_CFG,
				&enum_complete);
			break;
		case REQ_SET_DEV_CFG:
			LOG_DBG("req set cfg");
			err = usbh_req_set_cfg(dev, 1, 1);
			process_enum_req_resp(data, err, &retry_desc, REQ_NO, &enum_complete);
			break;
		case REQ_NO:
		default:
			atomic_clear_bit(&data->enum_meta_data.state, ENUM_IN_PROGRESS_STATUS);
			usbh_register_class(dev);

			if (class_data && class_data->connected) {
				err = class_data->connected(ctx);
			}
			enum_complete = true;

			if (!err) {
				enum_success = true;
			} else {
				/* de-register class */
				class_data = NULL;
				LOG_ERR("class registration failed");
			}
		}
	}

	if (enum_success) {
		printk("USB enumeration success\n");
	} else {
		atomic_clear_bit(&data->enum_meta_data.state, ENUM_IN_PROGRESS_STATUS);
		LOG_ERR("USB enumeration failed");
	}

	return err;
}

static int usbh_start_enumeration(struct usbh_contex *const ctx)
{
	const struct device *dev = ctx->dev;
	struct uhc_data *data = dev->data;

	k_msleep(USBH_DEV_CONN_DELAY);
	if (atomic_test_bit(&data->status, UHC_STATUS_DEV_CONN)) {
		k_thread_create(&usbh_enum_thread_data, usbh_enum_stack,
			K_KERNEL_STACK_SIZEOF(usbh_enum_stack), (k_thread_entry_t)usbh_enum_thread,
			(void *)ctx, NULL, NULL, K_PRIO_COOP(CONFIG_USBH_ENUM_PRIO), 0, K_NO_WAIT);

		k_thread_name_set(&usbh_enum_thread_data, "usbh_enum");
	} else {
		LOG_ERR("device connection false positive");
	}

	return 0;
}

static int usbh_free_enumeration_data(struct usbh_contex *const ctx)
{
	const struct device *dev = ctx->dev;
	struct uhc_data *data = dev->data;
	uint8_t i = 0;

	/* resetting UHC enable status */
	atomic_clear_bit(&data->status, UHC_STATUS_ENABLED);

	/* resetting enumeration meta data */
	memset((void *)&data->enum_meta_data, 0, sizeof(struct enumeration_meta_data));

	/* resetting descriptor data */
	memset((void *)&data->dev_descriptor, 0, sizeof(struct usb_device_descriptor));
	memset((void *)&data->cfg_descriptor, 0, sizeof(struct usb_cfg_descriptor));
	memset((void *)&data->if_descriptor, 0, sizeof(struct usb_if_descriptor));
	for (i = 0; i < CONFIG_UHC_MAX_IF_EP; i++) {
		memset((void *)&data->ep_descriptor[i], 0, sizeof(struct usb_ep_descriptor));
	}

	return 0;
}

static int ALWAYS_INLINE usbh_event_handler(struct usbh_contex *const ctx,
					    struct uhc_event *const event)
{
	int ret = 0;

	switch (event->type) {
	case UHC_EVT_DEV_CONNECTED_LS:
	case UHC_EVT_DEV_CONNECTED_FS:
	case UHC_EVT_DEV_CONNECTED_HS:
		LOG_INF("Device connected event");
		usbh_start_enumeration(ctx); /* does class registration as well */
		break;
	case UHC_EVT_DEV_REMOVED:
		LOG_INF("Device removed event");
		usbh_free_enumeration_data(ctx);
		if (class_data && class_data->removed) {
			ret = class_data->removed(ctx);
		}
		/* remove class registration */
		class_data = NULL;
		break;
	case UHC_EVT_RESETED:
		LOG_DBG("Bus reseted");
		/* TODO */
		if (class_data && class_data->removed) {
			ret = class_data->removed(ctx);
		}
		break;
	case UHC_EVT_SUSPENDED:
		LOG_DBG("Bus suspended");
		if (class_data && class_data->suspended) {
			ret = class_data->suspended(ctx);
		}
		break;
	case UHC_EVT_RESUMED:
		LOG_DBG("Bus resumed");
		if (class_data && class_data->resumed) {
			ret = class_data->resumed(ctx);
		}
		break;
	case UHC_EVT_RWUP:
		LOG_DBG("RWUP event");
		if (class_data && class_data->rwup) {
			ret = class_data->rwup(ctx);
		}
		break;
	case UHC_EVT_EP_REQUEST:
		event_ep_request(ctx, event);
		break;
	case UHC_EVT_ERROR:
		LOG_DBG("Error event");
		break;
	default:
		break;
	};

	return ret;
}

static void usbh_thread(const struct device *dev)
{
	struct uhc_event event;

	usbh_register_dev();

	while (true) {
		k_msgq_get(&usbh_msgq, &event, K_FOREVER);

		STRUCT_SECTION_FOREACH(usbh_contex, uhs_ctx)
		{
			if (uhs_ctx->dev == event.dev) {
				usbh_event_handler(uhs_ctx, &event);
			}
		}
	}
}

int usbh_init_device_intl(struct usbh_contex *const uhs_ctx)
{
	int ret;

	ret = uhc_init(uhs_ctx->dev, usbh_event_carrier);
	if (ret != 0) {
		LOG_ERR("Failed to init device driver");
		return ret;
	}

	return 0;
}

static int uhs_pre_init(void)
{
	k_thread_create(&usbh_thread_data, usbh_stack, K_KERNEL_STACK_SIZEOF(usbh_stack),
			(k_thread_entry_t)usbh_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_USBH_PRE_INIT_PRIO), 0, K_NO_WAIT);

	k_thread_name_set(&usbh_thread_data, "usbh");

	return 0;
}

SYS_INIT(uhs_pre_init, POST_KERNEL, CONFIG_USBH_INIT_PRIO);
