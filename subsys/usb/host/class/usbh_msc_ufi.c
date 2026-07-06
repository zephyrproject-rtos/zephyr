/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usbh_msc.h"
#include "usbh_msc_ufi.h"

extern int msc_bot_command(struct usbh_msc_data *msc,
			   const uint8_t *cmd, uint8_t cmd_len,
			   uint8_t *data, uint32_t data_len,
			   bool data_in);

int usbh_msc_test_unit_ready(struct usbh_msc_data *msc)
{
	uint8_t ufi_cmd[] = {
		UFI_TEST_UNIT_READY,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd), NULL, 0, false);
}

int usbh_msc_request_sense(struct usbh_msc_data *msc,
			   uint8_t *buffer,
			   uint8_t buffer_len)
{
	uint8_t ufi_cmd[] = {
		UFI_REQUEST_SENSE,
		0x00,
		0x00,
		0x00,
		buffer_len,
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       buffer, buffer_len, true);
}

int usbh_msc_inquiry(struct usbh_msc_data *msc,
		     uint8_t *buffer,
		     uint8_t buffer_len)
{
	uint8_t ufi_cmd[] = {
		UFI_INQUIRY,
		0x00,
		0x00,
		0x00,
		buffer_len,
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       buffer, buffer_len, true);
}

int usbh_msc_inquiry_vpd(struct usbh_msc_data *msc,
			 uint8_t page_code,
			 uint8_t *buffer,
			 uint8_t buffer_len)
{
	uint8_t ufi_cmd[] = {
		UFI_INQUIRY,
		0x01,
		page_code,
		0x00,
		buffer_len,
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       buffer, buffer_len, true);
}

int usbh_msc_mode_sense_6(struct usbh_msc_data *msc,
			  uint8_t page_code,
			  uint8_t *buffer,
			  uint8_t buffer_len)
{
	uint8_t ufi_cmd[] = {
		UFI_MODE_SENSE_6,
		0x00,
		page_code,
		0x00,
		buffer_len,
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       buffer, buffer_len, true);
}

int usbh_msc_mode_select(struct usbh_msc_data *msc,
			 uint8_t *buffer,
			 uint8_t buffer_len)
{
	uint8_t ufi_cmd[] = {
		UFI_MODE_SELECT,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		GET_BE_BYTE(buffer_len, 1),
		GET_BE_BYTE(buffer_len, 0),
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       buffer, buffer_len, false);
}

int usbh_msc_start_stop_unit(struct usbh_msc_data *msc,
			      uint8_t load_eject,
			      uint8_t start)
{
	uint8_t ufi_cmd[] = {
		UFI_START_STOP_UNIT,
		0x00,
		0x00,
		0x00,
		(uint8_t)((load_eject << 1) | start),
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd), NULL, 0, false);
}

int usbh_msc_prevent_allow_removal(struct usbh_msc_data *msc,
				   uint8_t prevent)
{
	uint8_t ufi_cmd[] = {
		UFI_PREVENT_ALLOW_REMOVAL,
		0x00,
		0x00,
		0x00,
		prevent,
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd), NULL, 0, false);
}

int usbh_msc_read_format_capacities(struct usbh_msc_data *msc,
				    uint8_t *buffer,
				    uint8_t buffer_len)
{
	uint8_t ufi_cmd[] = {
		UFI_READ_FORMAT_CAPACITIES,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		GET_BE_BYTE(buffer_len, 1),
		GET_BE_BYTE(buffer_len, 0),
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       buffer, buffer_len, true);
}

int usbh_msc_read_capacity(struct usbh_msc_data *msc,
			   uint8_t *buffer,
			   uint8_t buffer_len)
{
	uint8_t ufi_cmd[] = {
		UFI_READ_CAPACITY_10,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       buffer, buffer_len, true);
}

int usbh_msc_read10(struct usbh_msc_data *msc,
		    uint32_t block_address,
		    uint8_t *buffer,
		    uint32_t buffer_len,
		    uint16_t block_count)
{
	uint8_t ufi_cmd[] = {
		UFI_READ10,
		0x00,
		GET_BE_BYTE(block_address, 3),
		GET_BE_BYTE(block_address, 2),
		GET_BE_BYTE(block_address, 1),
		GET_BE_BYTE(block_address, 0),
		0x00,
		GET_BE_BYTE(block_count, 1),
		GET_BE_BYTE(block_count, 0),
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       buffer, buffer_len, true);
}

int usbh_msc_write10(struct usbh_msc_data *msc,
		     uint32_t block_address,
		     const uint8_t *buffer,
		     uint32_t buffer_len,
		     uint16_t block_count)
{
	uint8_t ufi_cmd[] = {
		UFI_WRITE10,
		0x00,
		GET_BE_BYTE(block_address, 3),
		GET_BE_BYTE(block_address, 2),
		GET_BE_BYTE(block_address, 1),
		GET_BE_BYTE(block_address, 0),
		0x00,
		GET_BE_BYTE(block_count, 1),
		GET_BE_BYTE(block_count, 0),
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       (uint8_t *)buffer, buffer_len, false);
}

int usbh_msc_read12(struct usbh_msc_data *msc,
		    uint32_t block_address,
		    uint8_t *buffer,
		    uint32_t buffer_len,
		    uint32_t block_count)
{
	uint8_t ufi_cmd[] = {
		UFI_READ12,
		0x00,
		GET_BE_BYTE(block_address, 3),
		GET_BE_BYTE(block_address, 2),
		GET_BE_BYTE(block_address, 1),
		GET_BE_BYTE(block_address, 0),
		GET_BE_BYTE(block_count, 3),
		GET_BE_BYTE(block_count, 2),
		GET_BE_BYTE(block_count, 1),
		GET_BE_BYTE(block_count, 0),
		0x00,
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       buffer, buffer_len, true);
}

int usbh_msc_write12(struct usbh_msc_data *msc,
		     uint32_t block_address,
		     const uint8_t *buffer,
		     uint32_t buffer_len,
		     uint32_t block_count)
{
	uint8_t ufi_cmd[] = {
		UFI_WRITE12,
		0x00,
		GET_BE_BYTE(block_address, 3),
		GET_BE_BYTE(block_address, 2),
		GET_BE_BYTE(block_address, 1),
		GET_BE_BYTE(block_address, 0),
		GET_BE_BYTE(block_count, 3),
		GET_BE_BYTE(block_count, 2),
		GET_BE_BYTE(block_count, 1),
		GET_BE_BYTE(block_count, 0),
		0x00,
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       (uint8_t *)buffer, buffer_len, false);
}

int usbh_msc_write_and_verify(struct usbh_msc_data *msc,
			       uint32_t block_address,
			       const uint8_t *buffer,
			       uint32_t buffer_len,
			       uint16_t block_count)
{
	uint8_t ufi_cmd[] = {
		UFI_WRITE_VERIFY,
		0x00,
		GET_BE_BYTE(block_address, 3),
		GET_BE_BYTE(block_address, 2),
		GET_BE_BYTE(block_address, 1),
		GET_BE_BYTE(block_address, 0),
		0x00,
		GET_BE_BYTE(block_count, 1),
		GET_BE_BYTE(block_count, 0),
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd),
			       (uint8_t *)buffer, buffer_len, false);
}

int usbh_msc_verify(struct usbh_msc_data *msc,
		    uint32_t block_address,
		    uint16_t verification_length)
{
	uint8_t ufi_cmd[] = {
		UFI_VERIFY,
		0x00,
		GET_BE_BYTE(block_address, 3),
		GET_BE_BYTE(block_address, 2),
		GET_BE_BYTE(block_address, 1),
		GET_BE_BYTE(block_address, 0),
		0x00,
		GET_BE_BYTE(verification_length, 1),
		GET_BE_BYTE(verification_length, 0),
		0x00
	};

	return msc_bot_command(msc, ufi_cmd, sizeof(ufi_cmd), NULL, 0, false);
}
