/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TUNING_USB_TRANSPORT_H
#define TUNING_USB_TRANSPORT_H

#define USB_TPORT_HID_REPORT_ID		(1)
#define USB_TPORT_HID_REPORT_COUNT	(56)
#define USB_TPORT_HID_REPORT_DATA_LEN	\
	(USB_TPORT_HID_REPORT_COUNT - sizeof(union usb_hid_report_hdr))

/* 4 byte header in every HID report */
union usb_hid_report_hdr {
	struct {
		uint8_t	report_id;
		uint8_t	unused[2];
		uint8_t	byte_count;
	} __packed byte;
	uint32_t	word;
};

typedef void (*usb_transport_receive_callback_t)(uint8_t *data, uint32_t len);

int usb_transport_init(usb_transport_receive_callback_t callback);
int usb_transport_send_reply(uint8_t *data, uint32_t len);

#endif /* TUNING_USB_TRANSPORT_H */
