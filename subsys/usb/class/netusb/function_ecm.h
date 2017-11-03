/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ecm_class_handler(struct usb_setup_packet *setup, s32_t *len, u8_t **data);


int ecm_send(struct net_pkt *pkt);
struct netusb_function *ecm_register_function(struct net_if *iface, u8_t in);
