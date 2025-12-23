/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "boot_header.h"
#include "fsl_common.h"

/******************************************************************************
 *  External references
 ******************************************************************************/
#if defined(CONFIG_BOARD_NXP_MCXE31X_BOOT_HEADER) && (CONFIG_BOARD_NXP_MCXE31X_BOOT_HEADER != 0U)

extern void *const _vector_start;

/******************************************************************************
 * Boot Header
 ******************************************************************************/
typedef struct image_vector_table {
	uint32_t header;                     /* header                         */
	uint32_t boot_config;                /* Boot configuration Word        */
	const uint32_t reserved1;            /* Reserved                       */
	const uint32_t *cm7_0_start_address; /* Start address of CM7_0 Core    */
	const uint32_t reserved2;            /* Reserved                       */
	const uint32_t *reserved3;           /* Reserved                       */
	const uint32_t reserved4;            /* Reserved                       */
	const uint32_t *reserved5;           /* Reserved                       */
	const uint32_t *reserved6;           /* Reserved                       */
	const uint32_t *lcc_config;          /* Address of LC config           */
	uint8_t reserved7[216];              /* Reserved for future use        */
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

const ivt_t _boot_header __attribute__((used, section(".boot_header"))) = {
	.header = 0x5AA55AA5,
	.boot_config = SET(CM7_0_ENABLE_MASK) | /* booting core is core0           */
		      CLR(BOOT_SEQ_MASK) |     /* unsecure boot is only supported */
		      CLR(APP_SWT_INIT_MASK),  /* SWT0 is not setup by BAF        */
	.cm7_0_start_address = (const uint32_t *)&_vector_start,
	.lcc_config = (const uint32_t *)&lc_config};

/******************************************************************************
 * Default configurations that can be overridden by strong definitions
 ******************************************************************************/

__WEAK const boot_lc_config_t lc_config = 0xffffffff;

#endif /* CONFIG_BOARD_NXP_MCXE31X_BOOT_HEADER */
/******************************************************************************
 * End of module
 ******************************************************************************/
