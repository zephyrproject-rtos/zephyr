/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB definitions
 */

#define NETUSB_MTU 1522

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

void netusb_recv(struct net_pkt *pkt);
int try_write(u8_t ep, u8_t *data, u16_t len);

void netusb_enable(const struct netusb_function *func);
void netusb_disable(void);
bool netusb_enabled(void);
