/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USB_HOST_CLASS_USBH_MSC_UFI_H_
#define ZEPHYR_SUBSYS_USB_HOST_CLASS_USBH_MSC_UFI_H_

#include <zephyr/kernel.h>

#define UFI_TEST_UNIT_READY        0x00
#define UFI_REQUEST_SENSE          0x03
#define UFI_INQUIRY                0x12
#define UFI_MODE_SENSE_6           0x1A
#define UFI_MODE_SELECT            0x15
#define UFI_START_STOP_UNIT        0x1B
#define UFI_PREVENT_ALLOW_REMOVAL  0x1E
#define UFI_READ_FORMAT_CAPACITIES 0x23
#define UFI_READ_CAPACITY_10       0x25
#define UFI_READ10                 0x28
#define UFI_WRITE10                0x2A
#define UFI_WRITE_VERIFY           0x2E
#define UFI_VERIFY                 0x2F
#define UFI_MODE_SENSE_10          0x5A
#define UFI_READ12                 0xA8
#define UFI_WRITE12                0xAA

#define VPD_SUPPORTED_PAGES        0x00
#define VPD_UNIT_SERIAL_NUMBER     0x80
#define VPD_DEVICE_IDENTIFICATION  0x83

#define GET_BE_BYTE(val, n) ((uint8_t)((val) >> ((n) * 8)))

struct scsi_inquiry_data {
	uint8_t peripheral_device_type : 5;
	uint8_t peripheral_qualifier : 3;
	uint8_t removable : 1;
	uint8_t reserved1 : 7;
	uint8_t version;
	uint8_t response_data_format : 4;
	uint8_t hisup : 1;
	uint8_t normaca : 1;
	uint8_t reserved2 : 2;
	uint8_t additional_length;
	uint8_t sccs : 1;
	uint8_t acc : 1;
	uint8_t tpgs : 2;
	uint8_t _3pc : 1;
	uint8_t reserved3 : 2;
	uint8_t protect : 1;
	uint8_t reserved4;
	uint8_t encserv : 1;
	uint8_t vs : 1;
	uint8_t multip : 1;
	uint8_t reserved5 : 5;
	char vendor_id[8];
	char product_id[16];
	char product_rev[4];
} __packed;

struct usbh_msc_data;

int usbh_msc_test_unit_ready(struct usbh_msc_data *msc);

int usbh_msc_request_sense(struct usbh_msc_data *msc,
			   uint8_t *buffer,
			   uint8_t buffer_len);

int usbh_msc_inquiry(struct usbh_msc_data *msc,
		     uint8_t *buffer,
		     uint8_t buffer_len);

int usbh_msc_inquiry_vpd(struct usbh_msc_data *msc,
			 uint8_t page_code,
			 uint8_t *buffer,
			 uint8_t buffer_len);

int usbh_msc_mode_sense_6(struct usbh_msc_data *msc,
			  uint8_t page_code,
			  uint8_t *buffer,
			  uint8_t buffer_len);

int usbh_msc_mode_select(struct usbh_msc_data *msc,
			 uint8_t *buffer,
			 uint8_t buffer_len);

int usbh_msc_start_stop_unit(struct usbh_msc_data *msc,
			      uint8_t load_eject,
			      uint8_t start);

int usbh_msc_prevent_allow_removal(struct usbh_msc_data *msc,
				   uint8_t prevent);

int usbh_msc_read_format_capacities(struct usbh_msc_data *msc,
				    uint8_t *buffer,
				    uint8_t buffer_len);

int usbh_msc_read_capacity(struct usbh_msc_data *msc,
			   uint8_t *buffer,
			   uint8_t buffer_len);

int usbh_msc_read10(struct usbh_msc_data *msc,
		    uint32_t block_address,
		    uint8_t *buffer,
		    uint32_t buffer_len,
		    uint16_t block_count);

int usbh_msc_write10(struct usbh_msc_data *msc,
		     uint32_t block_address,
		     const uint8_t *buffer,
		     uint32_t buffer_len,
		     uint16_t block_count);

int usbh_msc_read12(struct usbh_msc_data *msc,
		    uint32_t block_address,
		    uint8_t *buffer,
		    uint32_t buffer_len,
		    uint32_t block_count);

int usbh_msc_write12(struct usbh_msc_data *msc,
		     uint32_t block_address,
		     const uint8_t *buffer,
		     uint32_t buffer_len,
		     uint32_t block_count);

int usbh_msc_write_and_verify(struct usbh_msc_data *msc,
			       uint32_t block_address,
			       const uint8_t *buffer,
			       uint32_t buffer_len,
			       uint16_t block_count);

int usbh_msc_verify(struct usbh_msc_data *msc,
		    uint32_t block_address,
		    uint16_t verification_length);

#endif /* ZEPHYR_SUBSYS_USB_HOST_CLASS_USBH_MSC_UFI_H_ */
