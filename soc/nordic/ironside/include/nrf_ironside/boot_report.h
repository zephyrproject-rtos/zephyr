/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_BOOT_REPORT_H_
#define ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_BOOT_REPORT_H_

#include <stdint.h>
#include <stddef.h>

/** Constant used to check if an Nordic IronSide SE boot report has been written. */
#define IRONSIDE_BOOT_REPORT_MAGIC (0x4d69546fUL)

/** UICR had no errors. */
#define IRONSIDE_UICR_SUCCESS 0
/** There was an unexpected error processing the UICR. */
#define IRONSIDE_UICR_ERROR_UNEXPECTED 1
/** The UICR integrity check failed. */
#define IRONSIDE_UICR_ERROR_INTEGRITY 2
/** The UICR content check failed. */
#define IRONSIDE_UICR_ERROR_CONTENT 3
/** Failed to configure system based on UICR. */
#define IRONSIDE_UICR_ERROR_CONFIG 4
/** Unsupported UICR format version. */
#define IRONSIDE_UICR_ERROR_FORMAT 5

/** Error found in UICR.PROTECTEDMEM. */
#define IRONSIDE_UICR_REGID_PROTECTEDMEM 36
/** Error found in UICR.SECURESTORAGE. */
#define IRONSIDE_UICR_REGID_SECURESTORAGE 64
/** Error found in UICR.PERIPHCONF. */
#define IRONSIDE_UICR_REGID_PERIPHCONF 104
/** Error found in UICR.MPCCONF. */
#define IRONSIDE_UICR_REGID_MPCCONF 116
/** Error found in UICR.SECONDARY.ADDRESS/SIZE4KB */
#define IRONSIDE_UICR_REGID_SECONDARY 128
/** Error found in UICR.SECONDARY.PROTECTEDMEM. */
#define IRONSIDE_UICR_REGID_SECONDARY_PROTECTEDMEM 152
/** Error found in UICR.SECONDARY.PERIPHCONF. */
#define IRONSIDE_UICR_REGID_SECONDARY_PERIPHCONF 172
/** Error found in UICR.SECONDARY.MPCCONF. */
#define IRONSIDE_UICR_REGID_SECONDARY_MPCCONF 184

/** Failed to mount a CRYPTO secure storage partition in MRAM. */
#define IRONSIDE_UICR_SECURESTORAGE_ERROR_MOUNT_CRYPTO_FAILED 1
/** Failed to mount an ITS secure storage partition in MRAM. */
#define IRONSIDE_UICR_SECURESTORAGE_ERROR_MOUNT_ITS_FAILED 2
/** The start address and total size of all ITS partitions are not aligned to 4 KB. */
#define IRONSIDE_UICR_SECURESTORAGE_ERROR_MISALIGNED 3

/** There was an unexpected error processing UICR.PERIPHCONF. */
#define IRONSIDE_UICR_PERIPHCONF_ERROR_UNEXPECTED 1
/** The address contained in a UICR.PERIPHCONF array entry is not permitted. */
#define IRONSIDE_UICR_PERIPHCONF_ERROR_NOT_PERMITTED 2
/** The readback of the value for a UICR.PERIPHCONF array entry did not match. */
#define IRONSIDE_UICR_PERIPHCONF_ERROR_READBACK_MISMATCH 3

/** Booted in secondary mode. */
#define IRONSIDE_BOOT_MODE_FLAGS_SECONDARY_MASK 0x1

/** Booted normally by IronSide SE.*/
#define IRONSIDE_BOOT_REASON_DEFAULT 0
/** Booted because of a cpuconf service call by a different core. */
#define IRONSIDE_BOOT_REASON_CPUCONF_CALL 1
/** Booted in secondary mode because of a bootmode service call. */
#define IRONSIDE_BOOT_REASON_BOOTMODE_SECONDARY_CALL 2
/** Booted in secondary mode because of a boot error in the primary mode. */
#define IRONSIDE_BOOT_REASON_BOOTERROR 3
/** Booted in secondary mode because of local domain reset reason trigger. */
#define IRONSIDE_BOOT_REASON_TRIGGER_RESETREAS 4
/** Booted in secondary mode via the CTRL-AP. */
#define IRONSIDE_BOOT_REASON_CTRLAP_SECONDARYMODE 5

/** The boot had no errors. */
#define IRONSIDE_BOOT_ERROR_SUCCESS 0x0
/** The reset vector for the application firmware was not programmed. */
#define IRONSIDE_BOOT_ERROR_NO_APPLICATION_FIRMWARE 0x1
/** The IronSide SE was unable to parse the SysCtrl ROM report. */
#define IRONSIDE_BOOT_ERROR_ROM_REPORT_INVALID 0x2
/** The SysCtrl ROM booted the system in current limited mode due to an issue in the BICR. */
#define IRONSIDE_BOOT_ERROR_ROM_REPORT_CURRENT_LIMITED 0x3
/** The IronSide SE detected an issue with the HFXO configuration in the BICR. */
#define IRONSIDE_BOOT_ERROR_BICR_HFXO_INVALID 0x4
/** The IronSide SE detected an issue with the LFXO configuration in the BICR. */
#define IRONSIDE_BOOT_ERROR_BICR_LFXO_INVALID 0x5
/** The IronSide SE failed to boot the SysCtrl Firmware. */
#define IRONSIDE_BOOT_ERROR_SYSCTRL_START_FAILED 0x6
/** The UICR integrity check failed. */
#define IRONSIDE_BOOT_ERROR_UICR_INTEGRITY_FAILED 0x7
/** The UICR content is not valid */
#define IRONSIDE_BOOT_ERROR_UICR_CONTENT_INVALID 0x8
/** Integrity check of PROTECTEDMEM failed. */
#define IRONSIDE_BOOT_ERROR_UICR_PROTECTEDMEM_INTEGRITY_FAILED 0x9
/** Failed to configure system based on UICR. */
#define IRONSIDE_BOOT_ERROR_UICR_CONFIG_FAILED 0xA
/** The IronSide SE failed to mount its own storage. */
#define IRONSIDE_BOOT_ERROR_SECDOM_STORAGE_MOUNT_FAILED 0xB
/** Failed to initialize DVFS service */
#define IRONSIDE_BOOT_ERROR_DVFS_INIT_FAILED 0xC
/** Failed to boot secondary application firmware; configuration missing from UICR. */
#define IRONSIDE_BOOT_ERROR_NO_SECONDARY_APPLICATION_FIRMWARE 0xD
/** Integrity check of secondary PROTECTEDMEM failed. */
#define IRONSIDE_BOOT_ERROR_UICR_SECONDARY_PROTECTEDMEM_INTEGRITY_FAILED 0xE
/** Unsupported UICR format version. */
#define IRONSIDE_BOOT_ERROR_UICR_FORMAT_UNSUPPORTED 0xF
/** Value reserved for conditions that should never happen. */
#define IRONSIDE_BOOT_ERROR_UNEXPECTED 0xff

/** Index for RESETREAS.DOMAIN[NRF_DOMAIN_APPLICATION]. */
#define IRONSIDE_SECONDARY_RESETREAS_APPLICATION 0
/** Index for RESETREAS.DOMAIN[NRF_DOMAIN_RADIOCORE]. */
#define IRONSIDE_SECONDARY_RESETREAS_RADIOCORE 1

/** Length of the local domain context buffer in bytes. */
#define IRONSIDE_BOOT_REPORT_LOCAL_DOMAIN_CONTEXT_SIZE (16UL)
/** Length of the random data buffer in bytes. */
#define IRONSIDE_BOOT_REPORT_RANDOM_DATA_SIZE (32UL)

/** @brief Initialization/boot status description contained in the boot report. */
struct ironside_boot_report_init_status {
	/** Reserved for Future Use. */
	uint8_t rfu1[3];
	/** Boot error for the current boot (same as reported in BOOTSTATUS)*/
	uint8_t boot_error;
	/** Overall UICR status. */
	uint8_t uicr_status;
	/** Reserved for Future Use. */
	uint8_t rfu2;
	/** ID of the register that caused the error.
	 *  Only relevant for IRONSIDE_UICR_ERROR_CONTENT and IRONSIDE_UICR_ERROR_CONFIG.
	 */
	uint16_t uicr_regid;
	/** Additional description for IRONSIDE_UICR_ERROR_CONFIG. */
	union {
		/** UICR.SECURESTORAGE error description. */
		struct {
			/** Reason that UICR.SECURESTORAGE configuration failed. */
			uint16_t status;
			/** Owner ID of the failing secure storage partition.
			 *  Only relevant for IRONSIDE_UICR_SECURESTORAGE_ERROR_MOUNT_CRYPTO_FAILED
			 *  and IRONSIDE_UICR_SECURESTORAGE_ERROR_MOUNT_ITS_FAILED.
			 */
			uint16_t owner_id;
		} securestorage;
		/** UICR.PERIPHCONF error description. */
		struct {
			/** Reason that UICR.PERIPHCONF configuration failed. */
			uint16_t status;
			/** Index of the failing entry in the UICR.PERIPHCONF array. */
			uint16_t index;
		} periphconf;
	} uicr_detail;
};

/** @brief Initialization/boot context description contained in the boot report. */
struct ironside_boot_report_init_context {
	/** Reserved for Future Use */
	uint8_t rfu[3];
	/** Reason the processor was started. */
	uint8_t boot_reason;

	union {
		/** Data passed from booting local domain to local domain being booted.
		 *
		 * Valid if the boot reason is one of the following:
		 * - IRONSIDE_BOOT_REASON_CPUCONF_CALL
		 * - IRONSIDE_BOOT_REASON_BOOTMODE_SECONDARY_CALL
		 */
		uint8_t local_domain_context[IRONSIDE_BOOT_REPORT_LOCAL_DOMAIN_CONTEXT_SIZE];

		/** Initialiation error that triggered the boot.
		 *
		 * Valid if the boot reason is IRONSIDE_BOOT_REASON_BOOTERROR.
		 */
		struct ironside_boot_report_init_status trigger_init_status;

		/** RESETREAS.DOMAIN that triggered the boot.
		 *
		 * Valid if the boot reason is IRONSIDE_BOOT_REASON_TRIGGER_RESETREAS.
		 */
		uint32_t trigger_resetreas[4];
	};
};

/** @brief IronSide boot report. */
struct ironside_boot_report {
	/** Magic value used to identify valid boot report */
	uint32_t magic;
	/** Firmware version of IronSide SE. 8bit MAJOR.MINOR.PATCH.SEQNUM */
	uint32_t ironside_se_version_int;
	/** Human readable extraversion of IronSide SE */
	char ironside_se_extraversion[12];
	/** Firmware version of IronSide SE recovery firmware. 8bit MAJOR.MINOR.PATCH.SEQNUM */
	uint32_t ironside_se_recovery_version_int;
	/** Human readable extraversion of IronSide SE recovery firmware */
	char ironside_se_recovery_extraversion[12];
	/** Copy of SICR.UROT.UPDATE.STATUS.*/
	uint32_t ironside_update_status;
	/** Initialization/boot status. */
	struct ironside_boot_report_init_status init_status;
	/** Reserved for Future Use */
	uint16_t rfu1;
	/** Flags describing the current boot mode. */
	uint16_t boot_mode_flags;
	/** Data describing the context under which the CPU was booted. */
	struct ironside_boot_report_init_context init_context;
	/** CSPRNG data */
	uint8_t random_data[IRONSIDE_BOOT_REPORT_RANDOM_DATA_SIZE];
	/** Reserved for Future Use */
	uint32_t rfu2[64];
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
