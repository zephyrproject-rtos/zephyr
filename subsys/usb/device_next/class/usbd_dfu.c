/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usbd_msg.h"

#include <zephyr/init.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_dfu.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_dfu, CONFIG_USBD_DFU_LOG_LEVEL);

/*
 * It is very unlikely that anyone would need more than one instance of the DFU
 * class. Therefore, we make an exception here and do not support multiple
 * instances, which allows us to have a much simpler implementation.
 *
 * This implementation provides two class instances, one with a single
 * interface for the run-time mode, and the other with a number of user-defined
 * interfaces for the DFU mode. The DFU mode instance can have up to 256
 * (0...255) image (memory) segments, limited by the
 * CONFIG_USBD_DFU_NUMOF_IMAGES and maximum value of bAlternateSetting.
 *
 * The implementation implicitly sets the bitWillDetach flag and expects the
 * user to disable the device with run-time mode and enable a device with DFU
 * mode.
 */

#if defined(CONFIG_USBD_DFU_ENABLE_UPLOAD)
#define ATTR_CAN_UPLOAD USB_DFU_ATTR_CAN_UPLOAD
#else
#define ATTR_CAN_UPLOAD 0
#endif

#if defined(CONFIG_USBD_DFU_MANIFESTATION_TOLERANT)
#define ATTR_MANIFESTATION_TOLERANT USB_DFU_ATTR_MANIFESTATION_TOLERANT
#else
#define ATTR_MANIFESTATION_TOLERANT 0
#endif

/* DFU Functional Descriptor used for Run-Time und DFU mode */
static const struct usb_dfu_descriptor dfu_desc = {
	.bLength = sizeof(struct usb_dfu_descriptor),
	.bDescriptorType = USB_DESC_DFU_FUNCTIONAL,
	.bmAttributes = USB_DFU_ATTR_CAN_DNLOAD |
			ATTR_CAN_UPLOAD | ATTR_MANIFESTATION_TOLERANT |
			USB_DFU_ATTR_WILL_DETACH,
	.wDetachTimeOut = 0,
	.wTransferSize = sys_cpu_to_le16(CONFIG_USBD_DFU_TRANSFER_SIZE),
	.bcdDFUVersion = sys_cpu_to_le16(USB_DFU_VERSION),
};

/* Common class data for both run-time and DFU instances. */
struct usbd_dfu_data {
	struct usb_desc_header **const runtime_mode_descs;
	struct usb_desc_header **const dfu_mode_descs;
	enum usb_dfu_state state;
	enum usb_dfu_state next;
	enum usb_dfu_status status;
	struct k_work_delayable dwork;
	struct usbd_context *ctx;
	bool dfu_mode;
	struct usbd_dfu_image *image;
	uint8_t alternate;
};

/* Run-Time mode interface descriptor */
static __noinit struct usb_if_descriptor runtime_if0_desc;

/* Run-Time mode descriptors. No endpoints, identical for high and full speed. */
static struct usb_desc_header *runtime_mode_descs[] = {
	(struct usb_desc_header *) &runtime_if0_desc,
	(struct usb_desc_header *) &dfu_desc,
	NULL,
};

/*
 * DFU mode descriptors with two reserved indices for functional descriptor and
 * at least one for NULL. No endpoints, identical for high and full speed.
 */
static struct usb_desc_header *dfu_mode_descs[CONFIG_USBD_DFU_NUMOF_IMAGES + 2];

static struct usbd_dfu_data dfu_data = {
	.runtime_mode_descs = runtime_mode_descs,
	.dfu_mode_descs = dfu_mode_descs,
};

static const char *const dfu_state_list[] = {
	"APP_IDLE",
	"APP_DETACH",
	"DFU_IDLE",
	"DNLOAD_SYNC",
	"DNBUSY",
	"DNLOAD_IDLE",
	"MANIFEST_SYNC",
	"MANIFEST",
	"MANIFEST_WAIT_RST",
	"UPLOAD_IDLE",
	"ERROR",
};

static const char *const dfu_req_list[] = {
	"DETACH",
	"DNLOAD",
	"UPLOAD",
	"GETSTATUS",
	"CLRSTATUS",
	"GETSTATE",
	"ABORT",
};

BUILD_ASSERT(ARRAY_SIZE(dfu_state_list) == DFU_STATE_MAX,
	     "Number of entries in dfu_state_list is not equal to DFU_STATE_MAX");

BUILD_ASSERT(ARRAY_SIZE(dfu_req_list) == USB_DFU_REQ_ABORT + 1,
	     "Number of entries in dfu_req_list is not equal to USB_DFU_REQ_ABORT + 1");

static const char *dfu_state_string(const enum usb_dfu_state state)
{
	if (state >= 0 && state < DFU_STATE_MAX) {
		return dfu_state_list[state];
	}

	return "?";
}

static const char *dfu_req_string(const enum usb_dfu_state state)
{
	if (state >= 0 && state <= USB_DFU_REQ_ABORT) {
		return dfu_req_list[state];
	}

	return "?";
}

static void runtime_detach_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct usbd_dfu_data *data = CONTAINER_OF(dwork, struct usbd_dfu_data, dwork);

	usbd_msg_pub_simple(data->ctx, USBD_MSG_DFU_APP_DETACH, 0);
}

static void init_if_desc(struct usb_if_descriptor *const desc,
			 const uint8_t alternate, const uint8_t protocol)
{
	desc->bLength = sizeof(struct usb_if_descriptor);
	desc->bDescriptorType = USB_DESC_INTERFACE;
	desc->bInterfaceNumber = 0;
	desc->bAlternateSetting = alternate;
	desc->bNumEndpoints = 0;
	desc->bInterfaceClass = USB_BCC_APPLICATION;
	desc->bInterfaceSubClass = USB_DFU_SUBCLASS;
	desc->bInterfaceProtocol = protocol;
	desc->iInterface = 0;
}

static int usbd_dfu_preinit(void)
{
	struct usb_if_descriptor *if_desc;
	int n = 0;

	init_if_desc(&runtime_if0_desc, 0, USB_DFU_PROTOCOL_RUNTIME);

	STRUCT_SECTION_FOREACH(usbd_dfu_image, image) {
		if (n >= CONFIG_USBD_DFU_NUMOF_IMAGES) {
			LOG_ERR("Cannot register USB DFU image %s", image->name);
			return -ENOMEM;
		}

		if_desc = image->if_desc;
		init_if_desc(if_desc, n, USB_DFU_PROTOCOL_DFU);
		dfu_mode_descs[n] = (struct usb_desc_header *)if_desc;
		n++;
	}

	dfu_mode_descs[n] = (struct usb_desc_header *)&dfu_desc;

	k_work_init_delayable(&dfu_data.dwork, runtime_detach_work);

	return 0;
}

/*
 * Perhaps it makes sense to implement an on_registration class interface
 * callback and not use SYS_INIT().
 */
SYS_INIT(usbd_dfu_preinit, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

/*
 * This function is used for two purposes, to inform the image backend about
 * the next step and in some cases to get feedback if the next step is possible
 * from the image perspective.
 */
static inline bool usbd_dfu_image_next(struct usbd_class_data *const c_data,
				       const enum usb_dfu_state next)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);
	struct usbd_dfu_image *const image = data->image;

	if (image->next_cb != NULL) {
		return image->next_cb(image->priv, data->state, next);
	}

	return true;
}

static ALWAYS_INLINE void dfu_error(struct usbd_class_data *const c_data,
				    const enum usb_dfu_state next,
				    const enum usb_dfu_status status)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	data->next = next;
	data->status = status;
}

/*
 * Because some states (e.g. APP_IDLE, APP_DETACH) require a stall handshake to
 * be sent, but the state does not change to DFU_ERROR, there are some "return
 * -ENOTSUP" without state change to indicate a protocol error.
 */

static int app_idle_next(struct usbd_class_data *const c_data,
			 const struct usb_setup_packet *const setup)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	switch (setup->bRequest) {
	case USB_DFU_REQ_DETACH:
		data->next = APP_DETACH;
		return 0;
	case USB_DFU_REQ_GETSTATUS:
		__fallthrough;
	case USB_DFU_REQ_GETSTATE:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int app_detach_next(struct usbd_class_data *const c_data,
			   const struct usb_setup_packet *const setup)
{
	switch (setup->bRequest) {
	case USB_DFU_REQ_GETSTATUS:
		__fallthrough;
	case USB_DFU_REQ_GETSTATE:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int dfu_idle_next(struct usbd_class_data *const c_data,
			 const struct usb_setup_packet *const setup)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	switch (setup->bRequest) {
	case USB_DFU_REQ_DNLOAD:
		if (!(dfu_desc.bmAttributes & USB_DFU_ATTR_CAN_DNLOAD)) {
			dfu_error(c_data, DFU_ERROR, ERR_STALLEDPKT);
			return -ENOTSUP;
		}

		if (data->image == NULL || data->image->write_cb == NULL) {
			dfu_error(c_data, DFU_ERROR, ERR_VENDOR);
			return -ENOTSUP;
		}

		if (setup->wLength == 0) {
			dfu_error(c_data, DFU_ERROR, ERR_STALLEDPKT);
			return -ENOTSUP;
		}

		data->next = DFU_DNLOAD_SYNC;
		return 0;
	case USB_DFU_REQ_UPLOAD:
		if (!(dfu_desc.bmAttributes & USB_DFU_ATTR_CAN_UPLOAD)) {
			dfu_error(c_data, DFU_ERROR, ERR_STALLEDPKT);
			return -ENOTSUP;
		}

		if (data->image == NULL || data->image->read_cb == NULL) {
			dfu_error(c_data, DFU_ERROR, ERR_VENDOR);
			return -ENOTSUP;
		}

		if (setup->wLength > sys_le16_to_cpu(dfu_desc.wTransferSize)) {
			dfu_error(c_data, DFU_ERROR, ERR_STALLEDPKT);
			return -ENOTSUP;
		}

		data->next = DFU_UPLOAD_IDLE;
		return 0;
	case USB_DFU_REQ_ABORT:
		__fallthrough;
	case USB_DFU_REQ_GETSTATUS:
		__fallthrough;
	case USB_DFU_REQ_GETSTATE:
		return 0;
	default:
		dfu_error(c_data, DFU_ERROR, ERR_STALLEDPKT);
		return -ENOTSUP;
	}
}

static int dfu_dnload_sync_next(struct usbd_class_data *const c_data,
				const struct usb_setup_packet *const setup)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	switch (setup->bRequest) {
	case USB_DFU_REQ_GETSTATUS:
		/* Chack if image backend can change DFU_DNLOAD_SYNC -> DFU_DNLOAD_IDLE */
		if (usbd_dfu_image_next(c_data, DFU_DNLOAD_IDLE)) {
			data->next = DFU_DNLOAD_IDLE;
		} else {
			data->next = DFU_DNBUSY;
		}

		return 0;
	case USB_DFU_REQ_GETSTATE:
		return 0;
	default:
		dfu_error(c_data, DFU_ERROR, ERR_STALLEDPKT);
		return -ENOTSUP;
	}
}

static int dfu_dnbusy_next(struct usbd_class_data *const c_data,
			   const struct usb_setup_packet *const setup)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	/* Do not enforce bmPollTimeout (allow GET_STATUS immediately) */
	data->state = DFU_DNLOAD_SYNC;

	return dfu_dnload_sync_next(c_data, setup);
}

static int dfu_dnload_idle_next(struct usbd_class_data *const c_data,
				const struct usb_setup_packet *const setup)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	switch (setup->bRequest) {
	case USB_DFU_REQ_DNLOAD:
		if (setup->wLength == 0) {
			data->next = DFU_MANIFEST_SYNC;
		} else {
			data->next = DFU_DNLOAD_SYNC;
		}

		return 0;
	case USB_DFU_REQ_ABORT:
		data->next = DFU_IDLE;
		/* Notify image backend about DFU_DNLOAD_IDLE -> DFU_IDLE change */
		usbd_dfu_image_next(c_data, data->next);
	case USB_DFU_REQ_GETSTATUS:
		__fallthrough;
	case USB_DFU_REQ_GETSTATE:
		return 0;
	default:
		dfu_error(c_data, DFU_ERROR, ERR_STALLEDPKT);
		return -ENOTSUP;
	}
}

static int dfu_manifest_sync_next(struct usbd_class_data *const c_data,
				  const struct usb_setup_packet *const setup)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	switch (setup->bRequest) {
	case USB_DFU_REQ_GETSTATUS:
		if (usbd_dfu_image_next(c_data, DFU_IDLE)) {
			data->next = DFU_IDLE;
			usbd_msg_pub_simple(data->ctx, USBD_MSG_DFU_DOWNLOAD_COMPLETED, 0);
		} else {
			data->next = DFU_MANIFEST;
		}
	case USB_DFU_REQ_GETSTATE:
		return 0;
	default:
		dfu_error(c_data, DFU_ERROR, ERR_STALLEDPKT);
		return -ENOTSUP;
	}

	return 0;
}

static int dfu_manifest_next(struct usbd_class_data *const c_data,
			     const struct usb_setup_packet *const setup)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	/* Ignore poll timeout, proceed directly to next state. */

	if (dfu_desc.bmAttributes & USB_DFU_ATTR_MANIFESTATION_TOLERANT) {
		data->state = DFU_MANIFEST_SYNC;
		return dfu_manifest_sync_next(c_data, setup);
	}

	data->next = DFU_MANIFEST_WAIT_RST;
	usbd_dfu_image_next(c_data, DFU_MANIFEST_WAIT_RST);

	return 0;
}

static int dfu_manifest_wait_rst_next(struct usbd_class_data *const c_data,
				      const struct usb_setup_packet *const setup)
{
	/* Ignore all requests, wait for system or bus reset */

	return 0;
}

static int dfu_upload_idle_next(struct usbd_class_data *const c_data,
				const struct usb_setup_packet *const setup)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	switch (setup->bRequest) {
	case USB_DFU_REQ_UPLOAD:
		if (setup->wLength > sys_le16_to_cpu(dfu_desc.wTransferSize)) {
			dfu_error(c_data, DFU_ERROR, ERR_STALLEDPKT);
			return -ENOTSUP;
		}

		data->next = DFU_UPLOAD_IDLE;
		return 0;
	case USB_DFU_REQ_ABORT:
		data->next = DFU_IDLE;
		/* Notify image backend about DFU_UPLOAD_IDLE -> DFU_IDLE change */
		usbd_dfu_image_next(c_data, data->next);
	case USB_DFU_REQ_GETSTATUS:
		__fallthrough;
	case USB_DFU_REQ_GETSTATE:
		return 0;
	default:
		dfu_error(c_data, DFU_ERROR, ERR_STALLEDPKT);
		return -ENOTSUP;
	}
}

static int dfu_error_next(struct usbd_class_data *const c_data,
			  const struct usb_setup_packet *const setup)
{
	switch (setup->bRequest) {
	case USB_DFU_REQ_GETSTATUS:
		__fallthrough;
	case USB_DFU_REQ_GETSTATE:
		return 0;
	case USB_DFU_REQ_CLRSTATUS:
		dfu_error(c_data, DFU_IDLE, ERR_OK);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int (*next_entries[])(struct usbd_class_data *const c_data,
			     const struct usb_setup_packet *const setup) = {
	app_idle_next,
	app_detach_next,
	dfu_idle_next,
	dfu_dnload_sync_next,
	dfu_dnbusy_next,
	dfu_dnload_idle_next,
	dfu_manifest_sync_next,
	dfu_manifest_next,
	dfu_manifest_wait_rst_next,
	dfu_upload_idle_next,
	dfu_error_next,
};

BUILD_ASSERT(ARRAY_SIZE(next_entries) == DFU_STATE_MAX,
	     "Number of entries in next_entries is not equal to DFU_STATE_MAX");

/*
 * Here we only set the next state based on the current state, image state and
 * the new request. We do not copy/move any data and we do not update DFU state.
 *
 * The state change and additional actions are performed in four places, in the
 * host/device requests in runtime mode and in the host/device request in DFU
 * mode.
 */
static int dfu_set_next_state(struct usbd_class_data *const c_data,
			      const struct usb_setup_packet *const setup)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);
	int err;

	if (setup->RequestType.type != USB_REQTYPE_TYPE_CLASS) {
		return -ENOTSUP;
	}

	if (setup->bRequest >= ARRAY_SIZE(next_entries)) {
		return -ENOTSUP;
	}

	data->next = data->state;
	err = next_entries[data->state](c_data, setup);

	LOG_DBG("bRequest %s, state %s, next %s, error %d",
		dfu_req_string(setup->bRequest), dfu_state_string(data->state),
		dfu_state_string(data->next), err);

	return err;
}

/* Run-Time mode instance implementation, for instance "dfu_runtime" */

static int handle_get_status(struct usbd_class_data *const c_data,
			     const struct usb_setup_packet *const setup,
			     struct net_buf *const buf)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);
	size_t len = MIN(setup->wLength, net_buf_tailroom(buf));
	const size_t getstatus_len = 6;

	if (len != getstatus_len) {
		errno = -ENOTSUP;
		return 0;
	}

	/*
	 * Add GET_STATUS response consisting of
	 * bStatus, bwPollTimeout, bStatus, iString (no strings defined)
	 */
	net_buf_add_u8(buf, data->status);
	net_buf_add_le16(buf, CONFIG_USBD_DFU_POLLTIMEOUT);
	net_buf_add_u8(buf, 0);
	net_buf_add_u8(buf, data->state);
	net_buf_add_u8(buf, 0);

	return 0;
}

static int handle_get_state(struct usbd_class_data *const c_data,
			    const struct usb_setup_packet *const setup,
			    struct net_buf *const buf)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);
	size_t len = MIN(setup->wLength, net_buf_tailroom(buf));
	const size_t getstate_len = 1;

	if (len != getstate_len) {
		errno = -ENOTSUP;
		return 0;
	}

	net_buf_add_u8(buf, data->state);

	return 0;
}

static int runtime_mode_control_to_host(struct usbd_class_data *const c_data,
					const struct usb_setup_packet *const setup,
					struct net_buf *const buf)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	errno = dfu_set_next_state(c_data, setup);

	if (errno == 0) {
		switch (setup->bRequest) {
		case USB_DFU_REQ_GETSTATUS:
			errno = handle_get_status(c_data, setup, buf);
			break;
		case USB_DFU_REQ_GETSTATE:
			errno = handle_get_state(c_data, setup, buf);
			break;
		default:
			break;
		}
	}

	data->state = data->next;

	return 0;
}

static int runtime_mode_control_to_dev(struct usbd_class_data *const c_data,
				       const struct usb_setup_packet *const setup,
				       const struct net_buf *const buf)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	errno = dfu_set_next_state(c_data, setup);

	if (errno == 0) {
		if (setup->bRequest == USB_DFU_REQ_DETACH) {
			k_work_reschedule(&data->dwork, K_MSEC(100));
		}
	}

	data->state = data->next;

	return 0;
}

static void *runtime_mode_get_desc(struct usbd_class_data *const c_data,
				   const enum usbd_speed speed)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	return data->runtime_mode_descs;
}

static int runtime_mode_init(struct usbd_class_data *const c_data)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	LOG_DBG("Init class instance %p", c_data);
	data->dfu_mode = false;
	data->alternate = 0;
	data->state = APP_IDLE;
	data->next = APP_IDLE;
	data->image = NULL;
	data->ctx = usbd_class_get_ctx(c_data);

	return 0;
}

struct usbd_class_api runtime_mode_api = {
	.control_to_host = runtime_mode_control_to_host,
	.control_to_dev = runtime_mode_control_to_dev,
	.get_desc = runtime_mode_get_desc,
	.init = runtime_mode_init,
};

USBD_DEFINE_CLASS(dfu_runtime, &runtime_mode_api, &dfu_data, NULL);

/* DFU mode instance implementation, for instance "dfu_dfu" */

static int handle_upload(struct usbd_class_data *const c_data,
			 const struct usb_setup_packet *const setup,
			 struct net_buf *const buf)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);
	uint16_t size = MIN(setup->wLength, net_buf_tailroom(buf));
	struct usbd_dfu_image *const image = data->image;
	int ret;

	ret = image->read_cb(image->priv, setup->wValue, size, buf->data);
	if (ret >= 0) {
		net_buf_add(buf, ret);
		if (ret < sys_le16_to_cpu(dfu_desc.wTransferSize)) {
			data->state = DFU_IDLE;
		}
	} else {
		errno = -ENOTSUP;
		dfu_error(c_data, DFU_ERROR, ERR_UNKNOWN);
	}

	return 0;
}

static int handle_download(struct usbd_class_data *const c_data,
			   const struct usb_setup_packet *const setup,
			   const struct net_buf *const buf)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);
	struct usbd_dfu_image *const image = data->image;
	uint16_t size = MIN(setup->wLength, buf->len);
	int ret;

	ret = image->write_cb(image->priv, setup->wValue, size, buf->data);
	if (ret < 0) {
		errno = -ENOTSUP;
		dfu_error(c_data, DFU_ERROR, ERR_UNKNOWN);
	}

	return 0;
}

static int dfu_mode_control_to_host(struct usbd_class_data *const c_data,
				    const struct usb_setup_packet *const setup,
				    struct net_buf *const buf)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	errno = dfu_set_next_state(c_data, setup);

	if (errno == 0) {
		switch (setup->bRequest) {
		case USB_DFU_REQ_GETSTATUS:
			errno = handle_get_status(c_data, setup, buf);
			break;
		case USB_DFU_REQ_GETSTATE:
			errno = handle_get_state(c_data, setup, buf);
			break;
		case USB_DFU_REQ_UPLOAD:
			errno = handle_upload(c_data, setup, buf);
			break;
		default:
			break;
		}
	}

	data->state = data->next;

	return 0;
}

static int dfu_mode_control_to_dev(struct usbd_class_data *const c_data,
				   const struct usb_setup_packet *const setup,
				   const struct net_buf *const buf)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	errno = dfu_set_next_state(c_data, setup);

	if (errno == 0) {
		if (setup->bRequest == USB_DFU_REQ_DNLOAD) {
			handle_download(c_data, setup, buf);
		}
	}

	data->state = data->next;

	return 0;
}

static void dfu_mode_update(struct usbd_class_data *const c_data,
			    const uint8_t iface, const uint8_t alternate)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	LOG_DBG("Instance %p, interface %u alternate %u changed",
		c_data, iface, alternate);

	data->alternate = alternate;
	data->image = NULL;

	STRUCT_SECTION_FOREACH(usbd_dfu_image, image) {
		if (image->if_desc->bAlternateSetting == alternate) {
			data->image = image;
			break;
		}
	}
}

static void *dfu_mode_get_desc(struct usbd_class_data *const c_data,
			       const enum usbd_speed speed)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);

	return data->dfu_mode_descs;
}

static int dfu_mode_init(struct usbd_class_data *const c_data)
{
	struct usbd_dfu_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	LOG_DBG("Init class instance %p", c_data);
	data->dfu_mode = true;
	data->alternate = 0;
	data->image = NULL;
	data->state = DFU_IDLE;
	data->next = DFU_IDLE;
	data->ctx = usbd_class_get_ctx(c_data);

	STRUCT_SECTION_FOREACH(usbd_dfu_image, image) {
		if (image->if_desc->bAlternateSetting == data->alternate) {
			data->image = image;
		}

		if (usbd_add_descriptor(uds_ctx, image->sd_nd)) {
			LOG_ERR("Failed to add string descriptor");
		} else {
			image->if_desc->iInterface = usbd_str_desc_get_idx(image->sd_nd);
		}
	}

	return data->image == NULL ? -EINVAL : 0;
}

struct usbd_class_api dfu_api = {
	.control_to_host = dfu_mode_control_to_host,
	.control_to_dev = dfu_mode_control_to_dev,
	.update = dfu_mode_update,
	.get_desc = dfu_mode_get_desc,
	.init = dfu_mode_init,
};

USBD_DEFINE_CLASS(dfu_dfu, &dfu_api, &dfu_data, NULL);
