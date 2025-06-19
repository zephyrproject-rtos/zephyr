/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_BOOT_REPORT_H_
#define ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_BOOT_REPORT_H_

#include <stdint.h>
#include <stddef.h>

/** Constant used to check if an Nordic IronSide SE boot report has been written. */
#define IRONSIDE_BOOT_REPORT_MAGIC                     (0x4d69546fUL)
/** Length of the local domain context buffer in bytes. */
#define IRONSIDE_BOOT_REPORT_LOCAL_DOMAIN_CONTEXT_SIZE (16UL)
/** Length of the random data buffer in bytes. */
#define IRONSIDE_BOOT_REPORT_RANDOM_DATA_SIZE          (32UL)

/** @brief IronSide version structure. */
struct ironside_version {
	/** Wrapping sequence number ranging from 1-126, incremented for each release. */
	uint8_t seqnum;
	/** Path version. */
	uint8_t patch;
	/** Minor version. */
	uint8_t minor;
	/** Major version. */
	uint8_t major;
	/** Human readable extraversion string. */
	char extraversion[12];
};

/** @brief UICR error description contained in the boot report. */
struct ironside_boot_report_uicr_error {
	/** The type of error. A value of 0 indicates no error */
	uint32_t error_type;
	/** Error descriptions specific to each type of UICR error */
	union {
		/** RFU */
		struct {
			uint32_t rfu[4];
		} rfu;
	} description;
};

/** @brief IronSide boot report. */
struct ironside_boot_report {
	/** Magic value used to identify valid boot report */
	uint32_t magic;
	/** Firmware version of IronSide SE. */
	struct ironside_version ironside_se_version;
	/** Firmware version of IronSide SE recovery firmware. */
	struct ironside_version ironside_se_recovery_version;
	/** Copy of SICR.UROT.UPDATE.STATUS.*/
	uint32_t ironside_update_status;
	/** See @ref ironside_boot_report_uicr_error */
	struct ironside_boot_report_uicr_error uicr_error_description;
	/** Data passed from booting local domain to local domain being booted */
	uint8_t local_domain_context[IRONSIDE_BOOT_REPORT_LOCAL_DOMAIN_CONTEXT_SIZE];
	/** CSPRNG data */
	uint8_t random_data[IRONSIDE_BOOT_REPORT_RANDOM_DATA_SIZE];
	/** Reserved for Future Use */
	uint32_t rfu[64];
};

/**
 * @brief Get a pointer to the IronSide boot report.
 *
 * @param[out] report Will be set to point to the IronSide boot report.
 *
 * @retval 0 if successful.
 * @retval -EFAULT if the magic field in the report is incorrect.
 * @retval -EINVAL if @p report is NULL.
 */
int ironside_boot_report_get(const struct ironside_boot_report **report);

#endif /* ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_BOOT_REPORT_H_ */
