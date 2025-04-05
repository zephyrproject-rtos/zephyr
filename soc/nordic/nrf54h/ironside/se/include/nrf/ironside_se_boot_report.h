/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_NRF54H_IRONSIDE_SE_BOOT_REPORT_INCLUDE_NRF_IRONSIDE_SE_BOOT_REPORT_H_
#define ZEPHYR_SOC_NORDIC_NRF54H_IRONSIDE_SE_BOOT_REPORT_INCLUDE_NRF_IRONSIDE_SE_BOOT_REPORT_H_

#include <stdint.h>
#include <stddef.h>

#define IRONSIDE_SE_BOOT_REPORT_LOCAL_DOMAIN_CONTEXT_SIZE (16UL) /* Size in bytes */
#define IRONSIDE_SE_BOOT_REPORT_RANDOM_DATA_SIZE (32UL) /* Size in bytes */

/** @brief UICR error description contained in the boot report. */
struct ironside_se_boot_report_uicr_error {
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

/** @brief IRONside boot report. */
struct ironside_se_boot_report {
	/** Magic value used to identify valid boot report */
	uint32_t magic;
	/** Firmware version of IRONside SE. 8bit MAJOR.MINOR.PATCH.SEQNUM */
	uint32_t ironside_se_version_int;
	/** Human readable extraversion of IRONside SE */
	char ironside_se_extraversion[12];
	/** Firmware version of IRONside SE recovery firmware. 8bit MAJOR.MINOR.PATCH.SEQNUM */
	uint32_t ironside_se_recovery_version_int;
	/** Human readable extraversion of IRONside SE recovery firmware */
	char ironside_se_recovery_extraversion[12];
	/** Copy of SICR.UROT.UPDATE.STATUS.*/
	uint32_t ironside_update_status;
	/** See @ref ironside_se_boot_report_uicr_error */
	struct ironside_se_boot_report_uicr_error uicr_error_description;
	/** Data passed from booting local domain to local domain being booted */
	uint8_t local_domain_context[IRONSIDE_SE_BOOT_REPORT_LOCAL_DOMAIN_CONTEXT_SIZE];
	/** CSPRNG data */
	uint8_t random_data[IRONSIDE_SE_BOOT_REPORT_RANDOM_DATA_SIZE];
	/** Reserved for Future Use */
	uint32_t rfu[64];
};

/**
 * @brief Get a pointer to the IRONside boot report.
 *
 * @param[out] report Will be set to point to the IRONside boot report.
 *
 * @return non-negative value if success, negative value otherwise.
 * @retval -EFAULT if the magic field in the report is incorrect.
 * @retval -EINVAL if @ref report is NULL.
 */
int ironside_se_boot_report_get(const struct ironside_se_boot_report **report);

#endif /* ZEPHYR_SOC_NORDIC_NRF54H_IRONSIDE_SE_BOOT_REPORT_INCLUDE_NRF_IRONSIDE_SE_BOOT_REPORT_H_ */
