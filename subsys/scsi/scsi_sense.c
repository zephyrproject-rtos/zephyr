/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/scsi/scsi_sense.h>

int scsi_parse_sense(struct scsi_sense *sense, const uint8_t *buf, uint32_t len)
{
	uint8_t resp;

	if (sense == NULL || buf == NULL || len == 0U) {
		return -EINVAL;
	}

	(void)memset(sense, 0, sizeof(*sense));
	sense->response_code = buf[0];
	resp = buf[0] & 0x7fU;

	if (resp != 0x70U && resp != 0x71U && resp != 0x72U && resp != 0x73U) {
		if (len > sizeof(sense->raw)) {
			len = sizeof(sense->raw);
		}
		(void)memcpy(sense->raw, buf, len);
		sense->len = (uint8_t)len;
		return -ENOTSUP;
	}

	sense->key = buf[2] & 0x0fU;
	if (len >= 14U) {
		sense->asc = buf[12];
		sense->ascq = buf[13];
	} else if (len >= 13U) {
		sense->asc = buf[12];
	}

	if (len > sizeof(sense->raw)) {
		len = sizeof(sense->raw);
	}
	(void)memcpy(sense->raw, buf, len);
	sense->len = (uint8_t)len;

	return 0;
}

const char *scsi_sense_key_to_str(uint8_t key)
{
	switch (key) {
	case SCSI_SENSE_KEY_NO_SENSE:
		return "NO SENSE";
	case SCSI_SENSE_KEY_RECOVERED_ERROR:
		return "RECOVERED ERROR";
	case SCSI_SENSE_KEY_NOT_READY:
		return "NOT READY";
	case SCSI_SENSE_KEY_MEDIUM_ERROR:
		return "MEDIUM ERROR";
	case SCSI_SENSE_KEY_HARDWARE_ERROR:
		return "HARDWARE ERROR";
	case SCSI_SENSE_KEY_ILLEGAL_REQUEST:
		return "ILLEGAL REQUEST";
	case SCSI_SENSE_KEY_UNIT_ATTENTION:
		return "UNIT ATTENTION";
	case SCSI_SENSE_KEY_DATA_PROTECT:
		return "DATA PROTECT";
	case SCSI_SENSE_KEY_BLANK_CHECK:
		return "BLANK CHECK";
	case SCSI_SENSE_KEY_VENDOR_SPECIFIC:
		return "VENDOR SPECIFIC";
	case SCSI_SENSE_KEY_COPY_ABORTED:
		return "COPY ABORTED";
	case SCSI_SENSE_KEY_ABORTED_COMMAND:
		return "ABORTED COMMAND";
	default:
		return "UNKNOWN";
	}
}

const char *scsi_sense_asc_ascq_to_str(uint8_t asc, uint8_t ascq)
{
	if (asc == 0x00U && ascq == 0x00U) {
		return "no additional sense information";
	}
	if (asc == 0x04U && ascq == 0x01U) {
		return "logical unit is in process of becoming ready";
	}
	if (asc == 0x04U && ascq == 0x02U) {
		return "logical unit not ready, initializing command required";
	}
	if (asc == 0x08U && ascq == 0x01U) {
		return "logical unit communication failure";
	}
	if (asc == 0x0aU && ascq == 0x00U) {
		return "error log overflow";
	}
	if (asc == 0x20U && ascq == 0x00U) {
		return "invalid command operation code";
	}
	if (asc == 0x21U && ascq == 0x00U) {
		return "logical block address out of range";
	}
	if (asc == 0x27U && ascq == 0x00U) {
		return "write protected";
	}
	if (asc == 0x28U && ascq == 0x00U) {
		return "medium may have changed";
	}
	if (asc == 0x3aU && ascq == 0x00U) {
		return "medium not present";
	}

	return "additional sense code not mapped";
}
