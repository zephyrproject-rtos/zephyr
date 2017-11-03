/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB definitions
 */

struct netusb_function {
	int num_ep;
	struct usb_ep_cfg_data *ep;
	int (*connect_media)(bool status);
	int (*send_pkt)(struct net_pkt *pkt);
	int (*class_handler)(struct usb_setup_packet *setup, s32_t *len,
			     u8_t **data);
};

void netusb_recv(struct net_pkt *pkt);
int try_write(u8_t ep, u8_t *data, u16_t len);

#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
#define NETUSB_IFACE_IDX FIRST_IFACE_CDC_ECM
struct netusb_function ecm_function;
#endif
