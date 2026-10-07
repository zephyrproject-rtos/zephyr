/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCSI sense data parsing and diagnostics
 *
 * @since 4.5
 */

#ifndef ZEPHYR_INCLUDE_SCSI_SCSI_SENSE_H_
#define ZEPHYR_INCLUDE_SCSI_SCSI_SENSE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @name SCSI sense keys (SPC) */
/**@{*/
#define SCSI_SENSE_KEY_NO_SENSE        0x00U /*!< NO SENSE */
#define SCSI_SENSE_KEY_RECOVERED_ERROR 0x01U /*!< RECOVERED ERROR */
#define SCSI_SENSE_KEY_NOT_READY       0x02U /*!< NOT READY */
#define SCSI_SENSE_KEY_MEDIUM_ERROR    0x03U /*!< MEDIUM ERROR */
#define SCSI_SENSE_KEY_HARDWARE_ERROR  0x04U /*!< HARDWARE ERROR */
#define SCSI_SENSE_KEY_ILLEGAL_REQUEST 0x05U /*!< ILLEGAL REQUEST */
#define SCSI_SENSE_KEY_UNIT_ATTENTION  0x06U /*!< UNIT ATTENTION */
#define SCSI_SENSE_KEY_DATA_PROTECT    0x07U /*!< DATA PROTECT */
#define SCSI_SENSE_KEY_BLANK_CHECK     0x08U /*!< BLANK CHECK */
#define SCSI_SENSE_KEY_VENDOR_SPECIFIC 0x09U /*!< VENDOR SPECIFIC */
#define SCSI_SENSE_KEY_COPY_ABORTED    0x0aU /*!< COPY ABORTED */
#define SCSI_SENSE_KEY_ABORTED_COMMAND 0x0bU /*!< ABORTED COMMAND */
/**@}*/

/** Parsed fixed-format sense data */
struct scsi_sense {
	/** Sense data response code byte */
	uint8_t response_code;
	/** Sense key (@ref SCSI_SENSE_KEY_NO_SENSE and related constants) */
	uint8_t key;
	/** Additional sense code */
	uint8_t asc;
	/** Additional sense code qualifier */
	uint8_t ascq;
	/** Raw sense buffer copy */
	uint8_t raw[32];
	/** Valid length in @a raw */
	uint8_t len;
};

/**
 * @brief Parse fixed descriptor sense data (response 0x70–0x73)
 *
 * @param sense Output parsed sense structure
 * @param buf Raw sense buffer
 * @param len Length of @a buf in bytes
 *
 * @return 0 on success, negative errno on failure
 */
int scsi_parse_sense(struct scsi_sense *sense, const uint8_t *buf, uint32_t len);

/** @return Human-readable sense key name */
const char *scsi_sense_key_to_str(uint8_t key);

/** @return Human-readable ASC/ASCQ description (minimal mapping) */
const char *scsi_sense_asc_ascq_to_str(uint8_t asc, uint8_t ascq);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SCSI_SCSI_SENSE_H_ */
