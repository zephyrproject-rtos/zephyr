/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB definitions
 */

#define CDC_ECM_INT_EP_ADDR		0x83
#define CDC_ECM_IN_EP_ADDR		0x82
#define CDC_ECM_OUT_EP_ADDR		0x01

#define CDC_EEM_OUT_EP_ADDR		0x01
#define CDC_EEM_IN_EP_ADDR		0x82

#define RNDIS_INT_EP_ADDR		0x83
#define RNDIS_IN_EP_ADDR		0x82
#define RNDIS_OUT_EP_ADDR		0x01

struct netusb_function {
	int (*connect_media)(bool status);
	int (*send_pkt)(struct net_pkt *pkt);
};

struct net_if *netusb_net_iface(void);
void netusb_recv(struct net_pkt *pkt);

void netusb_enable(const struct netusb_function *func);
void netusb_disable(void);
bool netusb_enabled(void);
