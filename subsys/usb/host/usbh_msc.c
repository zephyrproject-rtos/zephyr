/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usbh_msc.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "usbh_ch9.h"
#include "usbh_device.h"
#if IS_ENABLED(CONFIG_USBH_MSC_CLASS)
#include "usbh_class.h"
#if IS_ENABLED(CONFIG_USBH_MSC_DISK)
#include <zephyr/usb/usb_msc_disk.h>
#endif
#endif

#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_msc, CONFIG_USBH_LOG_LEVEL);

#define EP_NUM_MASK 0x0FU

static bool msc_protocol_valid(uint8_t protocol)
{
	return protocol == USBH_MSC_PR_CBI || protocol == USBH_MSC_PR_CB ||
	       protocol == USBH_MSC_PR_BULK;
}

static int msc_collect_bulk_eps(const uint8_t *raw, uint16_t total, uint16_t ep_off,
				const struct usb_if_descriptor *ifd, struct usbh_msc_iface *out)
{
	uint16_t off = ep_off;
	uint8_t ep_in = 0U;
	uint8_t ep_out = 0U;
	uint8_t ep_int = 0U;
	uint8_t ep_int_num = 0U;
	uint8_t irq_interval = 0U;
	uint16_t mps_in = 0U;
	uint16_t mps_out = 0U;

	while (off + 2U <= total) {
		const uint8_t len = raw[off];
		const uint8_t type = raw[off + 1U];

		if (len < 2U || off + len > total) {
			return -EINVAL;
		}

		if (type == USB_DESC_INTERFACE) {
			break;
		}

		if (type == USB_DESC_ENDPOINT && len >= sizeof(struct usb_ep_descriptor)) {
			const struct usb_ep_descriptor *const ep = (const void *)(raw + off);
			const uint8_t tt = ep->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

			if (tt == USB_EP_TYPE_BULK) {
				const uint16_t mps =
					USB_MPS_EP_SIZE(sys_le16_to_cpu(ep->wMaxPacketSize));

				if (USB_EP_DIR_IS_IN(ep->bEndpointAddress)) {
					if (ep_in == 0U) {
						ep_in = ep->bEndpointAddress;
						mps_in = mps;
					}
				} else if (ep_out == 0U) {
					ep_out = ep->bEndpointAddress;
					mps_out = mps;
				}
			} else if (tt == USB_EP_TYPE_INTERRUPT) {
				if (ep_int == 0U) {
					ep_int = ep->bEndpointAddress;
					ep_int_num = ep->bEndpointAddress & EP_NUM_MASK;
					irq_interval = ep->bInterval;
				}
			}
		}

		off += len;
	}

	if (ep_in == 0U || ep_out == 0U) {
		return -ENOENT;
	}

	out->iface_num = ifd->bInterfaceNumber;
	out->alt_setting = ifd->bAlternateSetting;
	out->subclass = ifd->bInterfaceSubClass;
	out->protocol = ifd->bInterfaceProtocol;
	out->ep_in_addr = ep_in;
	out->ep_out_addr = ep_out;
	out->ep_in_num = ep_in & EP_NUM_MASK;
	out->ep_out_num = ep_out & EP_NUM_MASK;
	out->ep_int_addr = ep_int;
	out->ep_int_num = ep_int_num;
	out->irq_interval = irq_interval;
	out->mps_in = mps_in;
	out->mps_out = mps_out;
	out->bulk_only = (ifd->bInterfaceProtocol == USBH_MSC_PR_BULK);

	return 0;
}

static int msc_scan_configuration(const struct usb_device *udev, const uint8_t *raw, uint16_t total,
				  struct usbh_msc_iface *out)
{
	const struct usb_cfg_descriptor *const cfg = (const void *)raw;

	if (cfg->bDescriptorType != USB_DESC_CONFIGURATION || total < sizeof(*cfg)) {
		return -EINVAL;
	}

	if (out != NULL) {
		(void)memset(out, 0, sizeof(*out));
	}

#if IS_ENABLED(CONFIG_USBH_MSC_STRICT_PROBE)
	if (udev->dev_desc.bDeviceClass != USBH_MSC_DEV_CLASS_PER_INTERFACE) {
		return -ENOENT;
	}
#endif

	for (uint16_t off = 0U; off + 2U <= total;) {
		const uint8_t len = raw[off];
		const uint8_t type = raw[off + 1U];

		if (len < 2U || off + len > total) {
			break;
		}

		if (type == USB_DESC_INTERFACE && len >= sizeof(struct usb_if_descriptor)) {
			const struct usb_if_descriptor *const ifd = (const void *)(raw + off);
			struct usbh_msc_iface scratch;
			struct usbh_msc_iface *match = out != NULL ? out : &scratch;

			if (ifd->bInterfaceClass != USB_BCC_MASS_STORAGE) {
				off += len;
				continue;
			}

#if IS_ENABLED(CONFIG_USBH_MSC_STRICT_PROBE)
			if (ifd->bAlternateSetting != 0U) {
				off += len;
				continue;
			}

			if (ifd->bInterfaceSubClass < USBH_MSC_SC_MIN ||
			    ifd->bInterfaceSubClass > USBH_MSC_SC_MAX) {
				off += len;
				continue;
			}

			if (!msc_protocol_valid(ifd->bInterfaceProtocol)) {
				off += len;
				continue;
			}
#endif

			const int err = msc_collect_bulk_eps(raw, total, off + len, ifd, match);

			if (err != 0) {
				off += len;
				continue;
			}

#if IS_ENABLED(CONFIG_USBH_MSC_STRICT_PROBE)
			if (match->protocol == USBH_MSC_PR_CBI && match->ep_int_addr == 0U) {
				if (out != NULL) {
					(void)memset(out, 0, sizeof(*out));
				}
				off += len;
				continue;
			}

			if (!usbh_msc_subclass_scsi_supported(match->subclass)) {
				if (out != NULL) {
					LOG_DBG("MSC strict probe: subclass 0x%02x not "
						"UFI/8070/SCSI",
						match->subclass);
					(void)memset(out, 0, sizeof(*out));
					return -ENOTSUP;
				}
				off += len;
				continue;
			}
#endif

			if (out != NULL) {
				LOG_DBG("MSC if%u alt%u bulk IN=0x%02x OUT=0x%02x int=0x%02x "
					"mps %u/%u bbb=%d",
					out->iface_num, out->alt_setting, out->ep_in_addr,
					out->ep_out_addr, out->ep_int_addr, out->mps_in,
					out->mps_out, out->bulk_only ? 1 : 0);
			}

			return 0;
		}

		off += len;
	}

	return -ENOENT;
}

int usbh_msc_configuration_has_bulk(const struct usb_device *udev, const void *cfg_desc,
				    uint16_t len)
{
	if (udev == NULL || cfg_desc == NULL || len < sizeof(struct usb_cfg_descriptor)) {
		return -EINVAL;
	}

	return msc_scan_configuration(udev, cfg_desc, len, NULL);
}

int usbh_device_configuration_prefers(const struct usb_device *udev, const void *cfg_desc,
				      uint16_t len)
{
	return usbh_msc_configuration_has_bulk(udev, cfg_desc, len);
}

int usbh_msc_find_bulk_interface(const struct usb_device *udev, struct usbh_msc_iface *out)
{
	if (udev == NULL || out == NULL) {
		return -EINVAL;
	}

	if (udev->state != USB_STATE_CONFIGURED) {
		return -EPERM;
	}

	if (udev->cfg_desc == NULL) {
		return -EINVAL;
	}

	const uint8_t *const raw = udev->cfg_desc;
	const struct usb_cfg_descriptor *const cfg = (const void *)raw;
	const uint16_t total = sys_le16_to_cpu(cfg->wTotalLength);

	return msc_scan_configuration(udev, raw, total, out);
}

int usbh_msc_get_max_lun(struct usb_device *udev, uint8_t iface, uint8_t *max_lun_out)
{
	struct net_buf *buf;
	int err;

	if (udev == NULL || max_lun_out == NULL) {
		return -EINVAL;
	}

	if (udev->state != USB_STATE_CONFIGURED) {
		return -EPERM;
	}

	*max_lun_out = 0U;

	buf = usbh_xfer_buf_alloc(udev, 1U);
	if (buf == NULL) {
		return -ENOMEM;
	}

	err = usbh_req_setup(udev, USBH_MSC_BMREQ_GET_MAX_LUN, USBH_MSC_REQ_GET_MAX_LUN, 0, iface,
			     1U, buf);

	if (err == 0) {
		if (buf->len < 1U) {
			LOG_WRN("MSC GET_MAX_LUN: short data len=%u", buf->len);
			*max_lun_out = 0U;
		} else {
			*max_lun_out = buf->data[0];
			if (*max_lun_out > 15U) {
				LOG_WRN("MSC GET_MAX_LUN: invalid value 0x%02x, using 0",
					*max_lun_out);
				*max_lun_out = 0U;
			} else {
				LOG_INF("MSC GET_MAX_LUN: ok, max_lun=%u (%u logical unit(s))",
					*max_lun_out, (unsigned int)*max_lun_out + 1U);
			}
		}
	} else if (err == -ETIMEDOUT) {
		LOG_WRN("MSC GET_MAX_LUN: timeout (assume LUN 0 only)");
		err = 0;
	}

	if (err != 0) {
		/* STALL or failure => assume single LUN */
		LOG_INF("MSC GET_MAX_LUN: err=%d (assume LUN 0 only)", err);
		err = 0;
	}

	usbh_xfer_buf_free(udev, buf);

	return err;
}

int usbh_msc_prepare(struct usb_device *udev, struct usbh_msc_iface *out)
{
	int err = usbh_msc_find_bulk_interface(udev, out);

	if (err != 0) {
		return err;
	}

	out->max_lun = 0U;

	err = usbh_device_interface_set(udev, out->iface_num, 0U, false);
	if (err != 0) {
		return err;
	}

	if (!out->bulk_only) {
		return 0;
	}

	return usbh_msc_get_max_lun(udev, out->iface_num, &out->max_lun);
}

void usbh_msc_log_probe(const struct usb_device *udev)
{
#if IS_ENABLED(CONFIG_USBH_MSC_LOG_PROBE)
	struct usbh_msc_iface msc;
	const int ret = usbh_msc_find_bulk_interface(udev, &msc);

	if (ret != 0) {
		LOG_DBG("MSC probe: no bulk MSC interface (err=%d)", ret);
		return;
	}

	LOG_DBG("MSC probe: if%u alt%u sub=0x%02x proto=0x%02x IN=0x%02x(%u) OUT=0x%02x(%u) "
		"int=0x%02x mps=%u/%u BBB=%d (descriptor bind; GET_MAX_LUN in "
		"usbh_msc_prepare)",
		msc.iface_num, msc.alt_setting, msc.subclass, msc.protocol, msc.ep_in_addr,
		msc.ep_in_num, msc.ep_out_addr, msc.ep_out_num, msc.ep_int_addr, msc.mps_in,
		msc.mps_out, msc.bulk_only ? 1 : 0);
#else
	ARG_UNUSED(udev);
#endif
}

#if IS_ENABLED(CONFIG_USBH_MSC_CLASS)

static int usbh_msc_class_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
				const uint8_t iface)
{
	struct usbh_msc_iface msc;
	int ret;

	ARG_UNUSED(c_data);

	if (udev == NULL || udev->ctx == NULL) {
		return -EINVAL;
	}

	ret = usbh_msc_find_bulk_interface(udev, &msc);
	if (ret != 0) {
		return ret;
	}

	if (iface != USBH_CLASS_IFNUM_DEVICE && iface != msc.iface_num) {
		return -ENOENT;
	}

	usbh_msc_log_probe(udev);

#if IS_ENABLED(CONFIG_USBH_MSC_AUTO_BRINGUP)
	{
		struct usbh_context *const uhs_ctx = udev->ctx;

		ret = usbh_msc_storage_bringup(udev, uhs_ctx->dev, NULL);
	}
	if (ret != 0) {
		LOG_WRN("MSC class bringup failed: %d", ret);
		return ret;
	}
#endif

	return 0;
}

static int usbh_msc_class_removed(struct usbh_class_data *const c_data)
{
#if IS_ENABLED(CONFIG_USBH_MSC_DISK)
	if (c_data->udev != NULL) {
		usb_msc_disk_device_removed(c_data->udev);
	}
#else
	ARG_UNUSED(c_data);
#endif

	return 0;
}

static struct usbh_class_api usbh_msc_class_api = {
	.probe = usbh_msc_class_probe,
	.removed = usbh_msc_class_removed,
};

static struct usbh_class_filter usbh_msc_class_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_MASS_STORAGE,
		.sub = USBH_MSC_SC_SCSI,
		.proto = USBH_MSC_PR_BULK,
	},
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_MASS_STORAGE,
		.sub = USBH_MSC_SC_UFI,
		.proto = USBH_MSC_PR_BULK,
	},
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_MASS_STORAGE,
		.sub = USBH_MSC_SC_8070,
		.proto = USBH_MSC_PR_BULK,
	},
	{0},
};

USBH_DEFINE_CLASS(usbh_msc_class, &usbh_msc_class_api, NULL, usbh_msc_class_filters);

#endif /* CONFIG_USBH_MSC_CLASS */
