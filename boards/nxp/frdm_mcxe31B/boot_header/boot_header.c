/*
 * Copyright 2025 NXP
 *
 * SPDXLicense-Identifier: Apache-2.0
 */

#include "boot_header.h"
#include "fsl_common.h"

/******************************************************************************
 *  External references
 ******************************************************************************/
#if defined(BOOT_HEADER_ENABLE) && (BOOT_HEADER_ENABLE != 0U)

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
extern void *const __Vectors;
#elif defined(__MCUXPRESSO)
extern void *const g_pfnVectors;
#elif defined(__ICCARM__)
extern void *const __vector_table;
#elif defined(__GNUC__)
extern void *const _vector_start;
#else
#error Unsupported toolchain!
#endif

/******************************************************************************
 * Boot header
 ******************************************************************************/
typedef struct image_vector_table {
	uint32_t Header;                    /* Header                         */
	uint32_t BootConfig;                /* Boot configuration Word        */
	const uint32_t Reserved1;           /* Reserved                       */
	const uint32_t *CM7_0_StartAddress; /* Start address of CM7_0 Core    */
	const uint32_t Reserved2;           /* Reserved                       */
	const uint32_t *Reserved3;          /* Reserved                       */
	const uint32_t Reserved4;           /* Reserved                       */
	const uint32_t *Reserved5;          /* Reserved                       */
	const uint32_t *Reserved6;          /* Reserved                       */
	const uint32_t *LCConfig;           /* Address of LC config           */
	uint8_t Reserved7[216];             /* Reserved for future use        */
} ivt_t;

/******************************************************************************
 * SBAF definitions
 ******************************************************************************/
/* CM7_0_ENABLE:                                                              */
/* 0- Cortex-M7_0 application core clock gated after boot                     */
/* 1- Cortex-M7_0 application core clock un-gated after boot                  */
#define CM7_0_ENABLE_MASK 1U

/* Control the boot flow of the application:                                  */
/* 0- Non-Secure Boot- Application image is started by SBAF without any       */
/*    authentication in parallel to HSE firmware.                             */
/* 1- Secure Boot- Application image is executed by HSE firmware after the    */
/*    authentication. SBAF only starts the HSE firmware after successful      */
/*    authentication.                                                         */
#define BOOT_SEQ_MASK 8U

/* APP_SWT_INIT: Control SWT0 before starting application core(s):            */
/* 0- Disable.                                                                */
/* 1- Enable. SBAF initializes SWT0 before enabling application cores.        */
/*    SBAF scans this bit only when BOOT_SEQ bit is 0.                        */
#define APP_SWT_INIT_MASK 32U

/*!
 * @brief   Sets register field in peripheral configuration structure.
 * @details This macro sets register field <c>mask</c> in the peripheral
 *          configuration structure.
 * @param   mask  Register field to be set.
 * @note    Implemented as a macro.
 */
#define SET(mask) (mask)

/*!
 * @brief   Clears register field in peripheral configuration structure.
 * @details This macro clears register field <c>mask</c> in the peripheral
 *          configuration structure.
 * @param   mask  Register field to be cleared.
 * @note    Implemented as a macro.
 */
#define CLR(mask) 0

const ivt_t __boot_header __attribute__((used, section(".boot_header"))) = {
	.Header = 0x5AA55AA5,
	.BootConfig = SET(CM7_0_ENABLE_MASK) | /* booting core is core0           */
		      CLR(BOOT_SEQ_MASK) |     /* unsecure boot is only supported */
		      CLR(APP_SWT_INIT_MASK),  /* SWT0 is not setup by BAF        */
	.CM7_0_StartAddress =
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
		(const uint32_t *)&__Vectors,
#elif defined(__MCUXPRESSO)
		(const uint32_t *)&g_pfnVectors,
#elif defined(__ICCARM__)
		(const uint32_t *)&__vector_table,
#elif defined(__GNUC__)
		(const uint32_t *)&_vector_start,
#else
#error Unsupported toolchain!
#endif
	.LCConfig = (const uint32_t *)&LC_Config};

/******************************************************************************
 * Default configurations that can be overridden by strong definitions
 ******************************************************************************/

__WEAK const boot_lc_config_t LC_Config = 0xffffffff;

#endif /* BOOT_HEADER_ENABLE */
/******************************************************************************
 * End of module
 ******************************************************************************/
