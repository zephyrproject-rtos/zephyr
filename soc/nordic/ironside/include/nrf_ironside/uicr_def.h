/* SPDX-License-Identifier: LicenseRef-Nordic-4-Clause */
/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 */
#ifndef IRONSIDE_INCLUDE_UICR_DEF_H__
#define IRONSIDE_INCLUDE_UICR_DEF_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Common enum values */
#define UICR_MAGIC_ERASE_VALUE (0xBD2328A8UL)         /* Default erased value for all UICR fields */
#define UICR_DISABLED          UICR_MAGIC_ERASE_VALUE /* Common disabled value */
#define UICR_ENABLED           (0xFFFFFFFFUL)         /* Common enabled value */
#define UICR_PROTECTED         UICR_ENABLED           /*!< Common protected value */

/**
 * @brief APPROTECT [UICR_APPROTECT]
 */
struct UICR_APPROTECT {
	__IOM uint32_t APPLICATION; /* APPLICATION access port protection */
	__IOM uint32_t RADIOCORE;   /* RADIOCORE access port protection */

	__IM uint32_t RESERVED;

	__IOM uint32_t CORESIGHT; /* CoreSight access port protection */
};

/* APPROTECT APPLICATION enum values */
#define UICR_APPROTECT_APPLICATION_UNPROTECTED UICR_MAGIC_ERASE_VALUE
#define UICR_APPROTECT_APPLICATION_PROFILE1    (0x1730C77FUL)
#define UICR_APPROTECT_APPLICATION_PROFILE2    (0xF05F1363UL)
#define UICR_APPROTECT_APPLICATION_PROFILE3    (0x488C5ED6UL)
#define UICR_APPROTECT_APPLICATION_PROFILE4    (0x83C6B58DUL)
#define UICR_APPROTECT_APPLICATION_PROTECTED   UICR_PROTECTED

/* APPROTECT RADIOCORE enum values */
#define UICR_APPROTECT_RADIOCORE_UNPROTECTED UICR_MAGIC_ERASE_VALUE
#define UICR_APPROTECT_RADIOCORE_PROFILE1    (0x1730C77FUL)
#define UICR_APPROTECT_RADIOCORE_PROFILE2    (0xF05F1363UL)
#define UICR_APPROTECT_RADIOCORE_PROFILE3    (0x488C5ED6UL)
#define UICR_APPROTECT_RADIOCORE_PROFILE4    (0x83C6B58DUL)
#define UICR_APPROTECT_RADIOCORE_PROTECTED   UICR_PROTECTED

/* APPROTECT CORESIGHT enum values */
#define UICR_APPROTECT_CORESIGHT_UNPROTECTED UICR_MAGIC_ERASE_VALUE
#define UICR_APPROTECT_CORESIGHT_PROFILE1    (0x1730C77FUL)
#define UICR_APPROTECT_CORESIGHT_PROFILE2    (0xF05F1363UL)
#define UICR_APPROTECT_CORESIGHT_PROFILE3    (0x488C5ED6UL)
#define UICR_APPROTECT_CORESIGHT_PROFILE4    (0x83C6B58DUL)
#define UICR_APPROTECT_CORESIGHT_PROTECTED   UICR_PROTECTED

/**
 * @brief PROTECTEDMEM [UICR_PROTECTEDMEM] Protected Memory region configuration
 */
struct UICR_PROTECTEDMEM {
	__IOM uint32_t ENABLE;  /* Enable the Protected Memory region */
	__IOM uint32_t SIZE4KB; /* Protected Memory region size in 4 kiB blocks. */
};

/**
 * @brief WDTSTART [UICR_WDTSTART] Start a local watchdog timer ahead of the CPU boot.
 */
struct UICR_WDTSTART {
	__IOM uint32_t ENABLE;   /* Enable watchdog timer start. */
	__IOM uint32_t INSTANCE; /* Watchdog timer instance. */
	__IOM uint32_t CRV;      /* Initial CRV (Counter Reload Value) register value. */
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
struct UICR_SECURESTORAGE_SIZES {
	__IOM uint32_t APPLICATIONSIZE1KB; /* Size of the APPLICATION partition in 1 kiB blocks */
	__IOM uint32_t RADIOCORESIZE1KB;   /* Size of the RADIOCORE partition in 1 kiB blocks */
};

/**
 * @brief SECURESTORAGE [UICR_SECURESTORAGE] Secure Storage configuration
 */
struct UICR_SECURESTORAGE {
	__IOM uint32_t ENABLE;  /* Enable the Secure Storage */
	__IOM uint32_t ADDRESS; /* Start address of the Secure Storage region */

	__IOM struct UICR_SECURESTORAGE_SIZES
		CRYPTO; /* Secure Storage partitions for the Cryptographic service */
	__IOM struct UICR_SECURESTORAGE_SIZES
		ITS; /* Secure Storage partitions for the Internal Trusted Storage service */
};

/**
 * @brief PERIPHCONF [UICR_PERIPHCONF] Global domain peripheral configuration
 */
struct UICR_PERIPHCONF {
	__IOM uint32_t ENABLE;   /* Enable the global domain peripheral configuration */
	__IOM uint32_t ADDRESS;  /* Start address of the array of peripheral configuration entries*/
	__IOM uint32_t MAXCOUNT; /* Maximum number of peripheral configuration entries */
};

/**
 * @brief MPCCONF [UICR_MPCCONF] Global domain MPC configuration
 */
struct UICR_MPCCONF {
	__IOM uint32_t ENABLE;   /* Enable the global domain MPC configuration */
	__IOM uint32_t ADDRESS;  /* Start address of the array of MPC configuration entries*/
	__IOM uint32_t MAXCOUNT; /* Maximum number of MPC configuration entries */
};

/**
 * @brief TRIGGER [UICR_SECONDARY_TRIGGER] Automatic triggers for reset into secondary firmware
 */
struct UICR_SECONDARY_TRIGGER {
	__IOM uint32_t ENABLE; /* Enable automatic triggers for reset into secondary firmware*/

	__IOM uint32_t
		RESETREAS; /* Reset reasons that trigger automatic reset into secondary firmware*/
	__IM uint32_t RESERVED;
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
struct UICR_SECONDARY {
	__IOM uint32_t ENABLE;    /* Enable booting of secondary firmware. */
	__IOM uint32_t PROCESSOR; /* Processor to boot for the secondary firmware. */

	__IOM struct UICR_SECONDARY_TRIGGER
		TRIGGER;        /* Automatic triggers for reset into secondary firmware */
	__IOM uint32_t ADDRESS; /* Start address of the secondary firmware. This value is used as
				 * the initial value of the secure VTOR (Vector Table Offset
				 * Register) after CPU reset.
				 */
	__IOM struct UICR_PROTECTEDMEM
		PROTECTEDMEM; /* Protected Memory region for the secondary firmware.*/
	__IOM struct UICR_WDTSTART
		WDTSTART; /* Start a local watchdog timer ahead of the CPU boot. */
	__IOM struct UICR_PERIPHCONF PERIPHCONF; /* Global domain peripheral configuration used when
						  * booting the secondary firmware.
						  */
	__IOM struct UICR_MPCCONF MPCCONF; /* Global domain MPC configuration used when booting the
					    * secondary firmware
					    */
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
 * @brief User information configuration registers
 */
struct UICR {
	__IOM uint32_t VERSION; /* Version of the UICR format */
	__IM uint32_t RESERVED;
	__IOM uint32_t LOCK; /* Lock the UICR from modification */
	__IM uint32_t RESERVED1;
	__IOM struct UICR_APPROTECT APPROTECT;
	__IOM uint32_t ERASEPROTECT; /* Erase protection */
	__IOM struct UICR_PROTECTEDMEM PROTECTEDMEM;

	__IOM struct UICR_WDTSTART
		WDTSTART; /* Start a local watchdog timer ahead of the CPU boot. */
	__IM uint32_t RESERVED2;
	__IOM struct UICR_SECURESTORAGE SECURESTORAGE; /* Secure Storage configuration */
	__IM uint32_t RESERVED3[5];
	__IOM struct UICR_PERIPHCONF PERIPHCONF; /* Global domain peripheral configuration */
	__IOM struct UICR_MPCCONF MPCCONF;       /* Global domain MPC configuration */
	__IOM struct UICR_SECONDARY SECONDARY;   /* Secondary firmware configuration */
	__IOM uint32_t PADDING[15];
};

#ifdef __cplusplus
}
#endif
#endif /* IRONSIDE_INCLUDE_UICR_DEF_H__ */
