/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB definitions
 */

int netusb_send(struct net_pkt *pkt);

struct netusb_function {
	int (*connect_media)(bool status);
};

int try_write(u8_t ep, u8_t *data, u16_t len);
