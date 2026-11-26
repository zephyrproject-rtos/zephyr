/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/scsi/scsi_sense.h>

static void scsi_sense_store_raw(struct scsi_sense *sense, const uint8_t *buf, uint32_t len)
{
	if (len > sizeof(sense->raw)) {
		len = sizeof(sense->raw);
	}

	(void)memcpy(sense->raw, buf, len);
	sense->len = (uint8_t)len;
}

static int scsi_sense_parse_fixed(struct scsi_sense *sense, const uint8_t *buf, uint32_t len)
{
	uint32_t clip;

	if (len < 3U) {
		return -EINVAL;
	}

	sense->key = buf[2] & 0x0fU;

	clip = len;
	if (len > 7U) {
		uint32_t extra = buf[7];

		if (extra <= (UINT32_MAX - 8U)) {
			clip = MIN(len, extra + 8U);
		}
	}

	if (clip > 12U) {
		sense->asc = buf[12];
	}
	if (clip > 13U) {
		sense->ascq = buf[13];
	} else if (clip > 12U) {
		sense->ascq = 0U;
	}

	return 0;
}

static int scsi_sense_parse_descriptor(struct scsi_sense *sense, const uint8_t *buf, uint32_t len)
{
	if (len < 4U) {
		return -EINVAL;
	}

	sense->key = buf[1] & 0x0fU;
	sense->asc = buf[2];
	sense->ascq = buf[3];

	return 0;
}

int scsi_parse_sense(struct scsi_sense *sense, const uint8_t *buf, uint32_t len)
{
	uint8_t resp;
	int ret;

	if (sense == NULL || buf == NULL || len == 0U) {
		return -EINVAL;
	}

	(void)memset(sense, 0, sizeof(*sense));
	sense->response_code = buf[0];
	resp = buf[0] & 0x7fU;

	if (resp != 0x70U && resp != 0x71U && resp != 0x72U && resp != 0x73U) {
		scsi_sense_store_raw(sense, buf, len);
		return -ENOTSUP;
	}

	if (resp == 0x70U || resp == 0x71U) {
		ret = scsi_sense_parse_fixed(sense, buf, len);
	} else {
		ret = scsi_sense_parse_descriptor(sense, buf, len);
	}

	if (ret != 0) {
		scsi_sense_store_raw(sense, buf, len);
		return ret;
	}

	scsi_sense_store_raw(sense, buf, len);

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

bool scsi_sense_opcode_unsupported(const struct scsi_sense *sense)
{
	if (sense == NULL || sense->len == 0U) {
		return false;
	}

	if (sense->key != SCSI_SENSE_KEY_ILLEGAL_REQUEST) {
		return false;
	}

	return sense->asc == 0x20U;
}
