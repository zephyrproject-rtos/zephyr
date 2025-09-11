/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef IRONSIDE_INCLUDE_UICR_DEF_H__
#define IRONSIDE_INCLUDE_UICR_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Common enum values */
#define UICR_MAGIC_ERASE_VALUE (0xBD2328A8UL)         /* Default erased value for all UICR fields */
#define UICR_DISABLED          UICR_MAGIC_ERASE_VALUE /* Common disabled value */
#define UICR_ENABLED           (0xFFFFFFFFUL)         /* Common enabled value */
#define UICR_PROTECTED         UICR_ENABLED           /* Common protected value */

/**
 * @brief APPROTECT [UICR_APPROTECT]
 */
struct uicr_approtect {
	/* APPLICATION access port protection */
	volatile uint32_t application;
	/* RADIOCORE access port protection */
	volatile uint32_t radiocore;
	volatile const uint32_t reserved;
	/* CoreSight access port protection */
	volatile uint32_t coresight;
};

/* APPROTECT APPLICATION enum values */
#define UICR_APPROTECT_APPLICATION_UNPROTECTED UICR_MAGIC_ERASE_VALUE
#define UICR_APPROTECT_APPLICATION_PROTECTED   UICR_PROTECTED

/* APPROTECT RADIOCORE enum values */
#define UICR_APPROTECT_RADIOCORE_UNPROTECTED UICR_MAGIC_ERASE_VALUE
#define UICR_APPROTECT_RADIOCORE_PROTECTED   UICR_PROTECTED

/* APPROTECT CORESIGHT enum values */
#define UICR_APPROTECT_CORESIGHT_UNPROTECTED UICR_MAGIC_ERASE_VALUE
#define UICR_APPROTECT_CORESIGHT_PROTECTED   UICR_PROTECTED

/**
 * @brief PROTECTEDMEM [UICR_PROTECTEDMEM] Protected Memory region configuration
 */
struct uicr_protectedmem {
	/* Enable the Protected Memory region */
	volatile uint32_t enable;
	/* Protected Memory region size in 4 kiB blocks. */
	volatile uint32_t size4kb;
};

/**
 * @brief WDTSTART [UICR_WDTSTART] Start a local watchdog timer ahead of the CPU boot.
 */
struct uicr_wdtstart {
	/* Enable watchdog timer start. */
	volatile uint32_t enable;
	/* Watchdog timer instance. */
	volatile uint32_t instance;
	/* Initial CRV (Counter Reload Value) register value. */
	volatile uint32_t crv;
};

/* WDTSTART enum values */
#define UICR_WDTSTART_INSTANCE_WDT0 UICR_MAGIC_ERASE_VALUE
#define UICR_WDTSTART_INSTANCE_WDT1 (0x1730C77FUL)

/* WDTSTART CRV validation */
#define UICR_WDTSTART_CRV_CRV_MIN (0xFUL)
#define UICR_WDTSTART_CRV_CRV_MAX (0xFFFFFFFFUL)

/**
 * @brief SIZES [UICR_SECURESTORAGE_SIZES] Secure Storage partition sizes
 */
struct uicr_securestorage_sizes {
	/* Size of the APPLICATION partition in 1 kiB blocks */
	volatile uint32_t applicationsize1kb;
	/* Size of the RADIOCORE partition in 1 kiB blocks */
	volatile uint32_t radiocoresize1kb;
};

/**
 * @brief SECURESTORAGE [UICR_SECURESTORAGE] Secure Storage configuration
 */
struct uicr_securestorage {
	/* Enable the Secure Storage */
	volatile uint32_t enable;
	/* Start address of the Secure Storage region */
	volatile uint32_t address;
	/* Secure Storage partitions for the Cryptographic service */
	volatile struct uicr_securestorage_sizes crypto;
	/* Secure Storage partitions for the Internal Trusted Storage service */
	volatile struct uicr_securestorage_sizes its;
};

/**
 * @brief PERIPHCONF [UICR_PERIPHCONF] Global domain peripheral configuration
 */
struct uicr_periphconf {
	/* Enable the global domain peripheral configuration */
	volatile uint32_t enable;
	/* Start address of the array of peripheral configuration entries*/
	volatile uint32_t address;
	/* Maximum number of peripheral configuration entries */
	volatile uint32_t maxcount;
};

/**
 * @brief MPCCONF [UICR_MPCCONF] Global domain MPC configuration
 */
struct uicr_mpcconf {
	/* Enable the global domain MPC configuration */
	volatile uint32_t enable;
	/* Start address of the array of MPC configuration entries*/
	volatile uint32_t address;
	/* Maximum number of MPC configuration entries */
	volatile uint32_t maxcount;
};

/**
 * @brief TRIGGER [UICR_SECONDARY_TRIGGER] Automatic triggers for reset into secondary firmware
 */
struct uicr_secondary_trigger {
	/* Enable automatic triggers for reset into secondary firmware*/
	volatile uint32_t enable;
	/* Reset reasons that trigger automatic reset into secondary firmware*/
	volatile uint32_t resetreas;
	volatile const uint32_t reserved;
};

/* SECONDARY TRIGGER RESETREAS field access macros */

#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT0_Pos (0UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT0_Msk                                       \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT0_Pos)
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT1_Pos (1UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT1_Msk                                       \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONWDT1_Pos)
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONLOCKUP_Pos (3UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONLOCKUP_Msk                                     \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_APPLICATIONLOCKUP_Pos)
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT0_Pos (5UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT0_Msk                                         \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT0_Pos)
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT1_Pos (6UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT1_Msk                                         \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCOREWDT1_Pos)
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCORELOCKUP_Pos (8UL)
#define UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCORELOCKUP_Msk                                       \
	(0x1UL << UICR_SECONDARY_TRIGGER_RESETREAS_RADIOCORELOCKUP_Pos)

/**
 * @brief SECONDARY [UICR_SECONDARY] Secondary firmware configuration
 */
struct uicr_secondary {
	/* Enable booting of secondary firmware. */
	volatile uint32_t enable;
	/* Processor to boot for the secondary firmware. */
	volatile uint32_t processor;
	/* Automatic triggers for reset into secondary firmware */
	volatile struct uicr_secondary_trigger trigger;
	/* Start address of the secondary firmware. This value is used as
	 * the initial value of the secure VTOR (Vector Table Offset
	 * Register) after CPU reset.
	 */
	volatile uint32_t address;
	/* Protected Memory region for the secondary firmware.*/
	volatile struct uicr_protectedmem protectedmem;
	/* Start a local watchdog timer ahead of the CPU boot. */
	volatile struct uicr_wdtstart wdtstart;
	/* Global domain peripheral configuration used when
	 * booting the secondary firmware.
	 */
	volatile struct uicr_periphconf periphconf;
	/* Global domain MPC configuration used when booting the
	 * secondary firmware
	 */
	volatile struct uicr_mpcconf mpcconf;
};

/* SECONDARY enum values */
#define UICR_SECONDARY_PROCESSOR_APPLICATION UICR_MAGIC_ERASE_VALUE
#define UICR_SECONDARY_PROCESSOR_RADIOCORE   (0x1730C77FUL)

/* SECONDARY field access macros */
#define UICR_SECONDARY_ADDRESS_ADDRESS_Msk (0xFFFFF000UL)

/* UICR_VERSION: Version of the UICR format */

/* List of supported versions */
#define UICR_VERSION_2_0 0x00020000UL
#define UICR_VERSION_MAX UICR_VERSION_2_0

/* LOCK enum values */
#define UICR_LOCK_PALL_UNLOCKED                                                                    \
	UICR_MAGIC_ERASE_VALUE /* NVR page 0 can be written, and is not integrity                  \
				* checked by Nordic IronSide SE                                    \
				*/
#define UICR_LOCK_PALL_LOCKED                                                                      \
	(0xFFFFFFFFUL) /* NVR page 0 is read-only, and is integrity checked by                     \
			* Nordic IronSide SE on boot                                               \
			*/

/* ERASEPROTECT enum values */
#define UICR_ERASEPROTECT_PALL_UNPROTECTED UICR_MAGIC_ERASE_VALUE /* Erase protection disabled */
#define UICR_ERASEPROTECT_PALL_PROTECTED   UICR_PROTECTED         /* Erase protection enabled */

/**
 * @brief User information configuration region
 */
struct uicr {
	/* Version of the UICR format */
	volatile uint32_t version;
	volatile const uint32_t reserved;
	/* Lock the UICR from modification */
	volatile uint32_t lock;
	volatile const uint32_t reserved1;
	volatile struct uicr_approtect approtect;
	/* Erase protection */
	volatile uint32_t eraseprotect;
	volatile struct uicr_protectedmem protectedmem;
	/* Start a local watchdog timer ahead of the CPU boot. */
	volatile struct uicr_wdtstart wdtstart;
	volatile const uint32_t reserved2;
	/* Secure Storage configuration */
	volatile struct uicr_securestorage securestorage;
	volatile const uint32_t reserved3[5];
	/* Global domain peripheral configuration */
	volatile struct uicr_periphconf periphconf;
	/* Global domain MPC configuration */
	volatile struct uicr_mpcconf mpcconf;
	/* Secondary firmware configuration */
	volatile struct uicr_secondary secondary;
	volatile uint32_t padding[15];
};

#ifdef __cplusplus
}
#endif
#endif /* IRONSIDE_INCLUDE_UICR_DEF_H__ */
