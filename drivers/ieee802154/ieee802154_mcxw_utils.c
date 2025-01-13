/* ieee802154_mcxw_utils.c - NXP MCXW 802.15.4 driver utils*/

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <errno.h>

#include "ieee802154_mcxw_utils.h"

/* TODO IEEE 802.15.4 MAC Multipurpose frame format */
/* TODO add function checks */

enum offset_fcf_fields {
	OffsetFrameType = 0x00,
	OffsetSecurityEnabled = 0x03,
	OffsetFramePending = 0x04,
	OffsetAR = 0x05,
	OffsetPanIdCompression = 0x06,
	OffsetSeqNumberSuppression = 0x08,
	OffsetIEPresent = 0x09,
	OffsetDstAddrMode = 0x0A,
	OffsetFrameVersion = 0x0C,
	OffsetSrcAddrMode = 0x0E,
};

enum mask_fcf_fields {
	MaskFrameType = (0x7 << OffsetFrameType),
	MaskSecurityEnabled = (0x01 << OffsetSecurityEnabled),
	MaskFramePending = (0x01 << OffsetFramePending),
	MaskAR = (0x01 << OffsetAR),
	MaskPanIdCompression = (0x01 << OffsetPanIdCompression),
	MaskSeqNumberSuppression = (0x01 << OffsetSeqNumberSuppression),
	MaskIEPresent = (0x01 << OffsetIEPresent),
	MaskDstAddrMode = (0x03 << OffsetDstAddrMode),
	MaskFrameVersion = (0x03 << OffsetFrameVersion),
	MaskSrcAddrMode = (0x03 << OffsetSrcAddrMode),
};

enum modes_dst_addr {
	ModeDstAddrNone = 0x00,
	ModeDstAddrShort = (0x02 << OffsetDstAddrMode),
	ModeDstAddrExt = (0x03 << OffsetDstAddrMode),
};

enum version_frame {
	VersionIeee2003 = 0x00,
	VersionIeee2006 = 0x01,
	VersionIeee2015 = 0x02,
};

enum modes_src_addr {
	ModeSrcAddrNone = 0x00,
	ModeSrcAddrShort = (0x02 << OffsetSrcAddrMode),
	ModeSrcAddrExt = (0x03 << OffsetSrcAddrMode),
};

enum offset_scf_fields {
	OffsetSecurityLevel = 0x00,
	OffsetKeyIdMode = 0x03,
	OffsetFrameCntSuppression = 0x05,
	OffsetASNinNonce = 0x06,
};

enum mask_scf_fields {
	MaskSecurityLevel = (0x07 << OffsetSecurityLevel),
	MaskKeyIdMode = (0x03 << OffsetKeyIdMode),
	MaskFrameCntSuppression = (0x1 << OffsetFrameCntSuppression),
	MaskASNinNonce = (0x01 << OffsetASNinNonce),
};

static uint16_t get_frame_control_field(uint8_t *pdu, uint16_t length)
{
	if ((pdu == NULL) || (length < 3)) {
		return 0x00;
	}

	return (uint16_t)(pdu[0] | (pdu[1] << 8));
}

static bool is_security_enabled(uint16_t fcf)
{
	if (fcf) {
		return (bool)(fcf & MaskSecurityEnabled);
	}

	return false;
}

static bool is_ie_present(uint16_t fcf)
{
	if (fcf) {
		return (bool)(fcf & MaskIEPresent);
	}

	return false;
}

static uint8_t get_frame_version(uint16_t fcf)
{
	if (fcf) {
		return (uint8_t)((fcf & MaskFrameVersion) >> OffsetFrameVersion);
	}

	return 0xFF;
}

static bool is_frame_version_2015_fcf(uint16_t fcf)
{
	if (fcf) {
		return get_frame_version(fcf) == VersionIeee2015;
	}

	return false;
}

bool is_frame_version_2015(uint8_t *pdu, uint16_t length)
{
	uint16_t fcf = get_frame_control_field(pdu, length);

	if (fcf) {
		return get_frame_version(fcf) == VersionIeee2015;
	}

	return false;
}

static bool is_sequence_number_suppression(uint16_t fcf)
{
	if (fcf) {
		return (bool)(fcf & MaskSeqNumberSuppression);
	}

	return false;
}

static bool is_dst_panid_present(uint16_t fcf)
{
	bool present;

	if (!fcf) {
		return false;
	}

	if (is_frame_version_2015_fcf(fcf)) {
		switch (fcf & (MaskDstAddrMode | MaskSrcAddrMode | MaskPanIdCompression)) {
		case (ModeDstAddrNone | ModeSrcAddrNone):
		case (ModeDstAddrShort | ModeSrcAddrNone | MaskPanIdCompression):
		case (ModeDstAddrExt | ModeSrcAddrNone | MaskPanIdCompression):
		case (ModeDstAddrNone | ModeSrcAddrShort):
		case (ModeDstAddrNone | ModeSrcAddrExt):
		case (ModeDstAddrNone | ModeSrcAddrShort | MaskPanIdCompression):
		case (ModeDstAddrNone | ModeSrcAddrExt | MaskPanIdCompression):
		case (ModeDstAddrExt | ModeSrcAddrExt | MaskPanIdCompression):
			present = false;
			break;
		default:
			present = true;
		}
	} else {
		present = (bool)(fcf & MaskDstAddrMode);
	}

	return present;
}

static bool is_src_panid_present(uint16_t fcf)
{
	bool present;

	if (!fcf) {
		return false;
	}

	if (is_frame_version_2015_fcf(fcf)) {
		switch (fcf & (MaskDstAddrMode | MaskSrcAddrMode | MaskPanIdCompression)) {
		case (ModeDstAddrNone | ModeSrcAddrShort):
		case (ModeDstAddrNone | ModeSrcAddrExt):
		case (ModeDstAddrShort | ModeSrcAddrShort):
		case (ModeDstAddrShort | ModeSrcAddrExt):
		case (ModeDstAddrExt | ModeSrcAddrShort):
			present = true;
			break;
		default:
			present = false;
		}

	} else {
		present = ((fcf & MaskSrcAddrMode) != 0) && ((fcf & MaskPanIdCompression) == 0);
	}

	return present;
}

static uint8_t calculate_addr_field_size(uint16_t fcf)
{
	uint8_t size = 2;

	if (!fcf) {
		return 0;
	}

	if (!is_sequence_number_suppression(fcf)) {
		size += 1;
	}

	if (is_dst_panid_present(fcf)) {
		size += 2;
	}

	/* destination addressing mode */
	switch (fcf & MaskDstAddrMode) {
	case ModeDstAddrShort:
		size += 2;
		break;
	case ModeDstAddrExt:
		size += 8;
		break;
	default:
		break;
	}

	if (is_src_panid_present(fcf)) {
		size += 2;
	}

	/* source addressing mode */
	switch (fcf & MaskSrcAddrMode) {
	case ModeSrcAddrShort:
		size += 2;
		break;
	case ModeSrcAddrExt:
		size += 8;
		break;
	default:
		break;
	}

	return size;
}

static uint8_t get_keyid_mode(uint8_t *pdu, uint16_t length)
{
	uint16_t fcf = get_frame_control_field(pdu, length);
	uint8_t ash_start;

	if (is_security_enabled(fcf)) {
		ash_start = calculate_addr_field_size(fcf);
		return (uint8_t)((pdu[ash_start] & MaskKeyIdMode) >> OffsetKeyIdMode);
	}

	return 0xFF;
}

bool is_keyid_mode_1(uint8_t *pdu, uint16_t length)
{
	uint8_t key_mode = get_keyid_mode(pdu, length);

	if (key_mode == 0x01) {
		return true;
	}

	return false;
}

void set_frame_counter(uint8_t *pdu, uint16_t length, uint32_t fc)
{
	uint16_t fcf = get_frame_control_field(pdu, length);

	if (is_security_enabled(fcf)) {
		uint8_t ash_start = calculate_addr_field_size(fcf);
		uint8_t scf = pdu[ash_start];

		/* check that Frame Counter Suppression is not set */
		if (!(scf & MaskFrameCntSuppression)) {
			sys_put_le32(fc, &pdu[ash_start + 1]);
		}
	}
}

static uint8_t get_asn_size(uint8_t *pdu, uint16_t length)
{
	uint16_t fcf = get_frame_control_field(pdu, length);

	if (is_security_enabled(fcf)) {
		uint8_t ash_start = calculate_addr_field_size(fcf);
		uint8_t scf = pdu[ash_start];
		uint8_t size = 1;

		/* Frame Counter Suppression is not set */
		if (!(scf & MaskFrameCntSuppression)) {
			size += 4;
		}

		uint8_t key_mode = get_keyid_mode(pdu, length);

		switch (key_mode) {
		case 0x01:
			size += 1;
			break;
		case 0x02:
			size += 5;
		case 0x03:
			size += 9;
		default:
			break;
		}

		return size;
	}
	return 0;
}

static uint8_t *get_csl_ie_content_start(uint8_t *pdu, uint16_t length)
{
	uint16_t fcf = get_frame_control_field(pdu, length);

	if (is_ie_present(fcf)) {
		uint8_t ie_start_idx = calculate_addr_field_size(fcf) + get_asn_size(pdu, length);
		uint8_t *cur_ie = &pdu[ie_start_idx];

		uint8_t ie_header = (uint16_t)(cur_ie[0] | (cur_ie[1] << 8));
		uint8_t ie_length = ie_header & 0x7F;
		uint8_t ie_el_id = ie_header & 0x7F80;

		while ((ie_el_id != 0x7e) && (ie_el_id != 0x7f)) {
			if (ie_el_id == 0x1a) {
				return (cur_ie + 2);
			}
			cur_ie += (2 + ie_length);
			ie_header = (uint16_t)(cur_ie[0] | (cur_ie[1] << 8));
			ie_length = ie_header & 0x7F;
			ie_el_id = ie_header & 0x7F80;
		}
	}

	return NULL;
}

void set_csl_ie(uint8_t *pdu, uint16_t length, uint16_t period, uint16_t phase)
{
	uint8_t *csl_ie_content = get_csl_ie_content_start(pdu, length);

	if (csl_ie_content) {
		sys_put_le16(phase, csl_ie_content);
		sys_put_le16(period, csl_ie_content + 2);
	}
}
