/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_cdc_ecm_ethernet

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>

#include <eth.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdc_ecm, CONFIG_USBD_CDC_ECM_LOG_LEVEL);

#define CDC_ECM_EP_MPS_BULK		0
#define CDC_ECM_EP_MPS_INT		16
#define CDC_ECM_EP_INTERVAL_INT		0x0A

enum {
	CDC_ECM_IFACE_UP,
	CDC_ECM_CLASS_ENABLED,
	CDC_ECM_CLASS_SUSPENDED,
	CDC_ECM_OUT_ENGAGED,
};

/*
 * Transfers through two endpoints proceed in a synchronous manner,
 * with maximum block of NET_ETH_MAX_FRAME_SIZE.
 */
NET_BUF_POOL_FIXED_DEFINE(cdc_ecm_ep_pool,
			  DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 2,
			  NET_ETH_MAX_FRAME_SIZE,
			  sizeof(struct udc_buf_info), NULL);

struct cdc_ecm_notification {
	union {
		uint8_t bmRequestType;
		struct usb_req_type_field RequestType;
	};
	uint8_t bNotificationType;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} __packed;

struct cdc_ecm_eth_data {
	struct usbd_class_node *c_nd;
	struct usbd_desc_node *const mac_desc_nd;

	struct net_if *iface;
	uint8_t mac_addr[6];

	struct k_sem sync_sem;
	struct k_sem notif_sem;
	atomic_t state;
};

struct usbd_cdc_ecm_desc {
	struct usb_association_descriptor iad;

	struct usb_if_descriptor if0;
	struct cdc_header_descriptor if0_header;
	struct cdc_union_descriptor if0_union;
	struct cdc_ecm_descriptor if0_ecm;
	struct usb_ep_descriptor if0_int_ep;

	struct usb_if_descriptor if1_0;

	struct usb_if_descriptor if1_1;
	struct usb_ep_descriptor if1_1_in_ep;
	struct usb_ep_descriptor if1_1_out_ep;

	struct usb_desc_header nil_desc;
} __packed;

static uint8_t cdc_ecm_get_ctrl_if(struct usbd_class_node *const c_nd)
{
	struct usbd_cdc_ecm_desc *desc = c_nd->data->desc;

	return desc->if0.bInterfaceNumber;
}

static uint8_t cdc_ecm_get_int_in(struct usbd_class_node *const c_nd)
{
	struct usbd_cdc_ecm_desc *desc = c_nd->data->desc;

	return desc->if0_int_ep.bEndpointAddress;
}

static uint8_t cdc_ecm_get_bulk_in(struct usbd_class_node *const c_nd)
{
	struct usbd_cdc_ecm_desc *desc = c_nd->data->desc;

	return desc->if1_1_in_ep.bEndpointAddress;
}

static uint8_t cdc_ecm_get_bulk_out(struct usbd_class_node *const c_nd)
{
	struct usbd_cdc_ecm_desc *desc = c_nd->data->desc;

	return desc->if1_1_out_ep.bEndpointAddress;
}

static struct net_buf *cdc_ecm_buf_alloc(const uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&cdc_ecm_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;

	return buf;
}

/* Retrieve expected pkt size from ethernet/ip header */
static size_t ecm_eth_size(void *const ecm_pkt, const size_t len)
{
	uint8_t *ip_data = (uint8_t *)ecm_pkt + sizeof(struct net_eth_hdr);
	struct net_eth_hdr *hdr = (void *)ecm_pkt;
	uint16_t ip_len;

	if (len < NET_IPV6H_LEN + sizeof(struct net_eth_hdr)) {
		/* Too short */
		return 0;
	}

	switch (ntohs(hdr->type)) {
	case NET_ETH_PTYPE_IP:
		__fallthrough;
	case NET_ETH_PTYPE_ARP:
		ip_len = ntohs(((struct net_ipv4_hdr *)ip_data)->len);
		break;
	case NET_ETH_PTYPE_IPV6:
		ip_len = ntohs(((struct net_ipv6_hdr *)ip_data)->len);
		break;
	default:
		LOG_DBG("Unknown hdr type 0x%04x", hdr->type);
		return 0;
	}

	return sizeof(struct net_eth_hdr) + ip_len;
}

static int cdc_ecm_out_start(struct usbd_class_node *const c_nd)
{
	const struct device *dev = c_nd->data->priv;
	struct cdc_ecm_eth_data *data = dev->data;
	struct net_buf *buf;
	uint8_t ep;
	int ret;

	if (!atomic_test_bit(&data->state, CDC_ECM_CLASS_ENABLED)) {
		return -EACCES;
	}

	if (atomic_test_and_set_bit(&data->state, CDC_ECM_OUT_ENGAGED)) {
		return -EBUSY;
	}

	ep = cdc_ecm_get_bulk_out(c_nd);
	buf = cdc_ecm_buf_alloc(ep);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbd_ep_enqueue(c_nd, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
	}

	return  ret;
}

static int cdc_ecm_acl_out_cb(struct usbd_class_node *const c_nd,
			      struct net_buf *const buf, const int err)
{
	const struct device *dev = c_nd->data->priv;
	struct cdc_ecm_eth_data *data = dev->data;
	struct net_pkt *pkt;

	if (err || buf->len == 0) {
		goto restart_out_transfer;
	}

	/* Linux considers by default that network usb device controllers are
	 * not able to handle Zero Length Packet (ZLP) and then generates
	 * a short packet containing a null byte. Handle by checking the IP
	 * header length and dropping the extra byte.
	 */
	if (buf->data[buf->len - 1] == 0U) {
		/* Last byte is null */
		if (ecm_eth_size(buf->data, buf->len) == (buf->len - 1)) {
			/* last byte has been appended as delimiter, drop it */
			net_buf_remove_u8(buf);
		}
	}

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, buf->len,
					   AF_UNSPEC, 0, K_FOREVER);
	if (!pkt) {
		LOG_ERR("No memory for net_pkt");
		goto restart_out_transfer;
	}

	if (net_pkt_write(pkt, buf->data, buf->len)) {
		LOG_ERR("Unable to write into pkt");
		net_pkt_unref(pkt);
		goto restart_out_transfer;
	}

	LOG_DBG("Received packet len %zu", net_pkt_get_len(pkt));
	if (net_recv_data(data->iface, pkt) < 0) {
		LOG_ERR("Packet %p dropped by network stack", pkt);
		net_pkt_unref(pkt);
	}

restart_out_transfer:
	net_buf_unref(buf);
	atomic_clear_bit(&data->state, CDC_ECM_OUT_ENGAGED);

	return cdc_ecm_out_start(c_nd);
}

static int usbd_cdc_ecm_request(struct usbd_class_node *const c_nd,
				struct net_buf *buf, int err)
{
	struct usbd_contex *uds_ctx = c_nd->data->uds_ctx;
	const struct device *dev = c_nd->data->priv;
	struct cdc_ecm_eth_data *data = dev->data;
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);

	if (bi->ep == cdc_ecm_get_bulk_out(c_nd)) {
		return cdc_ecm_acl_out_cb(c_nd, buf, err);
	}

	if (bi->ep == cdc_ecm_get_bulk_in(c_nd)) {
		k_sem_give(&data->sync_sem);

		return 0;
	}

	if (bi->ep == cdc_ecm_get_int_in(c_nd)) {
		k_sem_give(&data->notif_sem);

		return 0;
	}

	return usbd_ep_buf_free(uds_ctx, buf);
}

static int cdc_ecm_send_notification(const struct device *dev,
				     const bool connected)
{
	struct cdc_ecm_eth_data *data = dev->data;
	struct usbd_class_node *c_nd = data->c_nd;
	struct cdc_ecm_notification notification = {
		.RequestType = {
			.direction = USB_REQTYPE_DIR_TO_HOST,
			.type = USB_REQTYPE_TYPE_CLASS,
			.recipient = USB_REQTYPE_RECIPIENT_INTERFACE,
		},
		.bNotificationType = USB_CDC_NETWORK_CONNECTION,
		.wValue = sys_cpu_to_le16((uint16_t)connected),
		.wIndex = sys_cpu_to_le16(cdc_ecm_get_ctrl_if(c_nd)),
		.wLength = 0,
	};
	struct net_buf *buf;
	uint8_t ep;
	int ret;

	if (!atomic_test_bit(&data->state, CDC_ECM_CLASS_ENABLED)) {
		LOG_INF("USB configuration is not enabled");
		return 0;
	}

	if (atomic_test_bit(&data->state, CDC_ECM_CLASS_SUSPENDED)) {
		LOG_INF("USB device is suspended (FIXME)");
		return 0;
	}

	ep = cdc_ecm_get_int_in(c_nd);
	buf = usbd_ep_buf_alloc(c_nd, ep, sizeof(struct cdc_ecm_notification));
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, &notification, sizeof(struct cdc_ecm_notification));
	ret = usbd_ep_enqueue(c_nd, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
		return ret;
	}

	k_sem_take(&data->notif_sem, K_FOREVER);
	net_buf_unref(buf);

	return 0;
}

static void usbd_cdc_ecm_update(struct usbd_class_node *const c_nd,
				const uint8_t iface, const uint8_t alternate)
{
	struct usbd_cdc_ecm_desc *desc = c_nd->data->desc;
	const uint8_t data_iface = desc->if1_1.bInterfaceNumber;
	const struct device *dev = c_nd->data->priv;
	struct cdc_ecm_eth_data *data = dev->data;

	LOG_INF("New configuration, interface %u alternate %u",
		iface, alternate);

	if (data_iface == iface && alternate == 0) {
		net_if_carrier_off(data->iface);
	}

	if (data_iface == iface && alternate == 1) {
		net_if_carrier_on(data->iface);
		if (cdc_ecm_out_start(c_nd)) {
			LOG_ERR("Failed to start OUT transfer");
		}

	}
}

static void usbd_cdc_ecm_enable(struct usbd_class_node *const c_nd)
{
	const struct device *dev = c_nd->data->priv;
	struct cdc_ecm_eth_data *data = dev->data;

	atomic_set_bit(&data->state, CDC_ECM_CLASS_ENABLED);
	LOG_INF("Configuration enabled");
}

static void usbd_cdc_ecm_disable(struct usbd_class_node *const c_nd)
{
	const struct device *dev = c_nd->data->priv;
	struct cdc_ecm_eth_data *data = dev->data;

	if (atomic_test_and_clear_bit(&data->state, CDC_ECM_CLASS_ENABLED)) {
		net_if_carrier_off(data->iface);
	}

	atomic_clear_bit(&data->state, CDC_ECM_CLASS_SUSPENDED);
	LOG_INF("Configuration disabled");
}

static void usbd_cdc_ecm_suspended(struct usbd_class_node *const c_nd)
{
	const struct device *dev = c_nd->data->priv;
	struct cdc_ecm_eth_data *data = dev->data;

	atomic_set_bit(&data->state, CDC_ECM_CLASS_SUSPENDED);
}

static void usbd_cdc_ecm_resumed(struct usbd_class_node *const c_nd)
{
	const struct device *dev = c_nd->data->priv;
	struct cdc_ecm_eth_data *data = dev->data;

	atomic_clear_bit(&data->state, CDC_ECM_CLASS_SUSPENDED);
}

static int usbd_cdc_ecm_ctd(struct usbd_class_node *const c_nd,
			    const struct usb_setup_packet *const setup,
			    const struct net_buf *const buf)
{
	if (setup->RequestType.recipient == USB_REQTYPE_RECIPIENT_INTERFACE &&
	    setup->bRequest == SET_ETHERNET_PACKET_FILTER) {
		LOG_INF("bRequest 0x%02x (SetPacketFilter) not implemented",
			setup->bRequest);

		return 0;
	}

	LOG_DBG("bmRequestType 0x%02x bRequest 0x%02x unsupported",
		setup->bmRequestType, setup->bRequest);
	errno = -ENOTSUP;

	return 0;
}

static int usbd_cdc_ecm_init(struct usbd_class_node *const c_nd)
{
	struct usbd_cdc_ecm_desc *desc = c_nd->data->desc;
	const uint8_t if_num = desc->if0.bInterfaceNumber;
	const struct device *dev = c_nd->data->priv;
	struct cdc_ecm_eth_data *const data = dev->data;

	/* Update relevant b*Interface fields */
	desc->iad.bFirstInterface = if_num;
	desc->if0_union.bControlInterface = if_num;
	desc->if0_union.bSubordinateInterface0 = if_num + 1;
	LOG_DBG("CDC ECM class initialized");

	if (usbd_add_descriptor(c_nd->data->uds_ctx, data->mac_desc_nd)) {
		LOG_ERR("Failed to add iMACAddress string descriptor");
	} else {
		desc->if0_ecm.iMACAddress = data->mac_desc_nd->idx;
	}

	return 0;
}

static void usbd_cdc_ecm_shutdown(struct usbd_class_node *const c_nd)
{
	struct usbd_cdc_ecm_desc *desc = c_nd->data->desc;
	const struct device *dev = c_nd->data->priv;
	struct cdc_ecm_eth_data *const data = dev->data;

	desc->if0_ecm.iMACAddress = 0;
	sys_dlist_remove(&data->mac_desc_nd->node);
}

static int cdc_ecm_send(const struct device *dev, struct net_pkt *const pkt)
{
	struct cdc_ecm_eth_data *const data = dev->data;
	struct usbd_class_node *c_nd = data->c_nd;
	size_t len = net_pkt_get_len(pkt);
	struct net_buf *buf;
	uint16_t bulk_mps;

	if (len > NET_ETH_MAX_FRAME_SIZE) {
		LOG_WRN("Trying to send too large packet, drop");
		return -ENOMEM;
	}

	if (!atomic_test_bit(&data->state, CDC_ECM_CLASS_ENABLED) ||
	    !atomic_test_bit(&data->state, CDC_ECM_IFACE_UP)) {
		LOG_INF("Configuration is not enabled or interface not ready");
		return -EACCES;
	}

	buf = cdc_ecm_buf_alloc(cdc_ecm_get_bulk_in(c_nd));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOMEM;
	}

	if (net_pkt_read(pkt, buf->data, len)) {
		LOG_ERR("Failed copy net_pkt");
		net_buf_unref(buf);

		return -ENOBUFS;
	}

	net_buf_add(buf, len);

	/*
	 * REVISE: It should be more abstract and
	 * not pull UDC stuff in the class code.
	 */
	if (udc_device_speed(c_nd->data->uds_ctx->dev) == UDC_BUS_SPEED_FS) {
		bulk_mps = 64;
	} else {
		bulk_mps = 512;
	}

	if (!(buf->len % bulk_mps)) {
		udc_ep_buf_set_zlp(buf);
	}

	usbd_ep_enqueue(c_nd, buf);
	k_sem_take(&data->sync_sem, K_FOREVER);
	net_buf_unref(buf);

	return 0;
}

static int cdc_ecm_set_config(const struct device *dev,
			      const enum ethernet_config_type type,
			      const struct ethernet_config *config)
{
	struct cdc_ecm_eth_data *data = dev->data;

	if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		memcpy(data->mac_addr, config->mac_address.addr,
		       sizeof(data->mac_addr));

		return 0;
	}

	return -ENOTSUP;
}

static int cdc_ecm_get_config(const struct device *dev,
			      enum ethernet_config_type type,
			      struct ethernet_config *config)
{
	return -ENOTSUP;
}

static enum ethernet_hw_caps cdc_ecm_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T;
}

static int cdc_ecm_iface_start(const struct device *dev)
{
	struct cdc_ecm_eth_data *data = dev->data;
	int ret;

	LOG_DBG("Start interface %p", data->iface);
	ret = cdc_ecm_send_notification(dev, true);
	if (!ret) {
		atomic_set_bit(&data->state, CDC_ECM_IFACE_UP);
	}

	return ret;
}

static int cdc_ecm_iface_stop(const struct device *dev)
{
	struct cdc_ecm_eth_data *data = dev->data;
	int ret;

	LOG_DBG("Stop interface %p", data->iface);
	ret = cdc_ecm_send_notification(dev, false);
	if (!ret) {
		atomic_clear_bit(&data->state, CDC_ECM_IFACE_UP);
	}

	return ret;
}

static void cdc_ecm_iface_init(struct net_if *const iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct cdc_ecm_eth_data *data = dev->data;

	data->iface = iface;
	ethernet_init(iface);
	net_if_set_link_addr(iface, data->mac_addr,
			     sizeof(data->mac_addr),
			     NET_LINK_ETHERNET);

	net_if_carrier_off(iface);

	LOG_DBG("CDC ECM interface initialized");
}

static int usbd_cdc_ecm_preinit(const struct device *dev)
{
	struct cdc_ecm_eth_data *data = dev->data;

	if (sys_get_le48(data->mac_addr) == sys_cpu_to_le48(0)) {
		gen_random_mac(data->mac_addr, 0, 0, 0);
	}

	LOG_DBG("CDC ECM device initialized");

	return 0;
}

static struct usbd_class_api usbd_cdc_ecm_api = {
	.request = usbd_cdc_ecm_request,
	.update = usbd_cdc_ecm_update,
	.enable = usbd_cdc_ecm_enable,
	.disable = usbd_cdc_ecm_disable,
	.suspended = usbd_cdc_ecm_suspended,
	.resumed = usbd_cdc_ecm_resumed,
	.control_to_dev = usbd_cdc_ecm_ctd,
	.init = usbd_cdc_ecm_init,
	.shutdown = usbd_cdc_ecm_shutdown,
};

static const struct ethernet_api cdc_ecm_eth_api = {
	.iface_api.init = cdc_ecm_iface_init,
	.get_config = cdc_ecm_get_config,
	.set_config = cdc_ecm_set_config,
	.get_capabilities = cdc_ecm_get_capabilities,
	.send = cdc_ecm_send,
	.start = cdc_ecm_iface_start,
	.stop = cdc_ecm_iface_stop,
};

#define CDC_ECM_DEFINE_DESCRIPTOR(n)						\
static struct usbd_cdc_ecm_desc cdc_ecm_desc_##n = {				\
	.iad = {								\
		.bLength = sizeof(struct usb_association_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,			\
		.bFirstInterface = 0,						\
		.bInterfaceCount = 0x02,					\
		.bFunctionClass = USB_BCC_CDC_CONTROL,				\
		.bFunctionSubClass = ECM_SUBCLASS,				\
		.bFunctionProtocol = 0,						\
		.iFunction = 0,							\
	},									\
										\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 1,						\
		.bInterfaceClass = USB_BCC_CDC_CONTROL,				\
		.bInterfaceSubClass = ECM_SUBCLASS,				\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if0_header = {								\
		.bFunctionLength = sizeof(struct cdc_header_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = HEADER_FUNC_DESC,				\
		.bcdCDC = sys_cpu_to_le16(USB_SRN_1_1),				\
	},									\
										\
	.if0_union = {								\
		.bFunctionLength = sizeof(struct cdc_union_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UNION_FUNC_DESC,				\
		.bControlInterface = 0,						\
		.bSubordinateInterface0 = 1,					\
	},									\
										\
	.if0_ecm = {								\
		.bFunctionLength = sizeof(struct cdc_ecm_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = ETHERNET_FUNC_DESC,			\
		.iMACAddress = 0,						\
		.bmEthernetStatistics = sys_cpu_to_le32(0),			\
		.wMaxSegmentSize = sys_cpu_to_le16(NET_ETH_MAX_FRAME_SIZE),	\
		.wNumberMCFilters = sys_cpu_to_le16(0),				\
		.bNumberPowerFilters = 0,					\
	},									\
										\
	.if0_int_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = sys_cpu_to_le16(CDC_ECM_EP_MPS_INT),		\
		.bInterval = CDC_ECM_EP_INTERVAL_INT,				\
	},									\
										\
	.if1_0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 1,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 0,						\
		.bInterfaceClass = USB_BCC_CDC_DATA,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if1_1 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 1,						\
		.bAlternateSetting = 1,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_CDC_DATA,				\
		.bInterfaceSubClass = ECM_SUBCLASS,				\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if1_1_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x82,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(CDC_ECM_EP_MPS_BULK),		\
		.bInterval = 0,							\
	},									\
										\
	.if1_1_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(CDC_ECM_EP_MPS_BULK),		\
		.bInterval = 0,							\
	},									\
										\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
}

#define USBD_CDC_ECM_DT_DEVICE_DEFINE(n)					\
	CDC_ECM_DEFINE_DESCRIPTOR(n);						\
	USBD_DESC_STRING_DEFINE(mac_desc_nd_##n,				\
				DT_INST_PROP(n, remote_mac_address),		\
				USBD_DUT_STRING_INTERFACE);			\
										\
	static struct usbd_class_data usbd_cdc_ecm_data_##n;			\
										\
	USBD_DEFINE_CLASS(cdc_ecm_##n,						\
			  &usbd_cdc_ecm_api,					\
			  &usbd_cdc_ecm_data_##n);				\
										\
	static struct cdc_ecm_eth_data eth_data_##n = {				\
		.c_nd = &cdc_ecm_##n,						\
		.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),		\
		.sync_sem = Z_SEM_INITIALIZER(eth_data_##n.sync_sem, 0, 1),	\
		.notif_sem = Z_SEM_INITIALIZER(eth_data_##n.notif_sem, 0, 1),	\
		.mac_desc_nd = &mac_desc_nd_##n,				\
	};									\
										\
	static struct usbd_class_data usbd_cdc_ecm_data_##n = {			\
		.desc = (struct usb_desc_header *)&cdc_ecm_desc_##n,		\
		.priv = (void *)DEVICE_DT_GET(DT_DRV_INST(n)),			\
	};									\
										\
	ETH_NET_DEVICE_DT_INST_DEFINE(n, usbd_cdc_ecm_preinit, NULL,		\
		&eth_data_##n, NULL,						\
		CONFIG_ETH_INIT_PRIORITY,					\
		&cdc_ecm_eth_api,						\
		NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(USBD_CDC_ECM_DT_DEVICE_DEFINE);
