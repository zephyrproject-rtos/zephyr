/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ecm_class_handler(struct usb_setup_packet *setup, s32_t *len, u8_t **data);

void ecm_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status);
void ecm_bulk_out(u8_t ep, enum usb_dc_ep_cb_status_code ep_status);
void ecm_bulk_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status);

int ecm_send(struct net_pkt *pkt);
struct netusb_function *ecm_register_function(struct net_if *iface, u8_t in);
