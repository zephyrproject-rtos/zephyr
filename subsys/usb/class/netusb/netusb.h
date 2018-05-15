/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB definitions
 */

#define NETUSB_MTU 1500

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

#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
#define NETUSB_IFACE_IDX FIRST_IFACE_CDC_ECM
struct netusb_function ecm_function;
#elif defined(CONFIG_USB_DEVICE_NETWORK_RNDIS)
#define NETUSB_IFACE_IDX FIRST_IFACE_RNDIS
struct netusb_function rndis_function;
#elif defined(CONFIG_USB_DEVICE_NETWORK_EEM)
#define NETUSB_IFACE_IDX FIRST_IFACE_CDC_EEM
struct netusb_function eem_function;
#else
#error Unknown USB Device Networking function
#endif
