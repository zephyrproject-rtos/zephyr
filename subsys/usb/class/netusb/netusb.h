/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB definitions
 */

#define NETUSB_MTU 1500

#define CDC_ECM_INT_EP_ADDR		0x83
#define CDC_ECM_IN_EP_ADDR		0x82
#define CDC_ECM_OUT_EP_ADDR		0x01

#define CDC_EEM_OUT_EP_ADDR		0x01
#define CDC_EEM_IN_EP_ADDR		0x82

#define RNDIS_INT_EP_ADDR		0x83
#define RNDIS_IN_EP_ADDR		0x82
#define RNDIS_OUT_EP_ADDR		0x01

struct netusb_function {
	int num_ep;
	struct usb_ep_cfg_data *ep;

	int (*init)(void);
	int (*connect_media)(bool status);
	int (*send_pkt)(struct net_pkt *pkt);
	int (*class_handler)(struct usb_setup_packet *setup, s32_t *len,
			     u8_t **data);
	void (*status_cb)(enum usb_dc_status_code status, u8_t *param);
};

void netusb_recv(struct net_pkt *pkt);
int try_write(u8_t ep, u8_t *data, u16_t len);

void netusb_enable(void);
void netusb_disable(void);
u8_t netusb_get_first_iface_number(void);

#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
struct netusb_function ecm_function;
#elif defined(CONFIG_USB_DEVICE_NETWORK_RNDIS)
struct netusb_function rndis_function;
#elif defined(CONFIG_USB_DEVICE_NETWORK_EEM)
struct netusb_function eem_function;
#else
#error Unknown USB Device Networking function
#endif
