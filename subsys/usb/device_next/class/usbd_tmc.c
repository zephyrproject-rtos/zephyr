#include <zephyr/usb/class/usb_tmc.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

static void *usbd_tmc_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	struct usbd_tmc_data *data = usbd_class_get_private(c_data);

	ARG_UNUSED(speed);

	return data->fs_desc;
}

static int usbd_tmc_init(struct usbd_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static int usbd_tmc_control_to_host(struct usbd_class_data *const c_data,
				    const struct usb_setup_packet *const setup,
				    struct net_buf *const buf)
{
	ARG_UNUSED(c_data);

	if (setup->RequestType.type != USB_REQTYPE_TYPE_CLASS) {
		errno = -ENOTSUP;
		return 0;
	}

	switch (setup->bRequest) {

	case USBTMC_REQ_GET_CAPABILITIES: {
		static struct usb_tmc_capabilities caps = {
			.Status = USBTMC_STATUS_SUCCESS,
			.bcdUSBTMC = sys_cpu_to_le16(0x0100),
			.InterfaceCapabilities = 0,
			.DeviceCapabilities = 0,
		};

		net_buf_add_mem(buf, &caps, MIN(sizeof(caps), setup->wLength));
		return 0;
	}

	default:
		errno = -ENOTSUP;
		return 0;
	}
}

static const struct usbd_class_api usbd_tmc_api = {
	.get_desc = usbd_tmc_get_desc,
	.init = usbd_tmc_init,
	.control_to_host = usbd_tmc_control_to_host,
};

#define TMC_DEFINE_DESCRIPTOR(n)                                                                   \
	static struct usb_tmc_desc tmc_desc_##n = {                                                \
		.if0 =                                                                             \
			{                                                                          \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 0,                                             \
				.bAlternateSetting = 0,                                            \
				.bNumEndpoints = 2,                                                \
				.bInterfaceClass = USB_TMC_CLASS,                                  \
				.bInterfaceSubClass = USB_TMC_SUBCLASS,                            \
				.bInterfaceProtocol = USB_TMC_PROTOCOL_BASE,                       \
				.iInterface = 0,                                                   \
			},                                                                         \
		.if0_ep_out =                                                                      \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x01,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64),                             \
				.bInterval = 0,                                                    \
			},                                                                         \
		.if0_ep_in =                                                                       \
			{                                                                          \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x81,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64),                             \
				.bInterval = 0,                                                    \
			},                                                                         \
		.nil_desc =                                                                        \
			{                                                                          \
				.bLength = 0,                                                      \
				.bDescriptorType = 0,                                              \
			},                                                                         \
	}

#define TMC_DEFINE_DESC_LIST(n)                                                                    \
	static const struct usb_desc_header *tmc_fs_desc_##n[] = {                                 \
		(struct usb_desc_header *)&tmc_desc_##n.if0,                                       \
		(struct usb_desc_header *)&tmc_desc_##n.if0_ep_out,                                \
		(struct usb_desc_header *)&tmc_desc_##n.if0_ep_in,                                 \
		(struct usb_desc_header *)&tmc_desc_##n.nil_desc,                                  \
	}

#define TMC_DEFINE_DATA(n)                                                                         \
	static struct usbd_tmc_data tmc_data_##n = {                                               \
		.desc = &tmc_desc_##n,                                                             \
		.fs_desc = tmc_fs_desc_##n,                                                        \
	}

#define TMC_DEFINE_CLASS(n) USBD_DEFINE_CLASS(tmc_##n, &usbd_tmc_api, &tmc_data_##n, NULL)

TMC_DEFINE_DESCRIPTOR(0);
TMC_DEFINE_DESC_LIST(0);
TMC_DEFINE_DATA(0);
TMC_DEFINE_CLASS(0);