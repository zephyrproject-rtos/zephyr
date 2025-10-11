/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(pse84_s_sau_h)
#define pse84_s_sau_h

#include <infineon_kconfig.h>
#include "cmsis_compiler.h"
#include "cy_device.h"

#define CY_SAU_REGION_CNT     (3U)
#define CY_SAU_MAX_REGION_CNT (8U)

typedef struct {
	uint8_t reg_num;    /* Region number. */
	uint32_t base_addr; /* Base address of SAU region. */
	uint32_t size;      /* Size of SAU region. */
	uint32_t end_addr;  /* End address of SAU region. */
	bool nsc;           /* Is this region Non-Secure Callable? */
} cy_sau_config_t;

void cy_sau_init(void);
extern const cy_sau_config_t sau_config[CY_SAU_REGION_CNT];

#endif /* #if !defined(pse84_s_sau_h) */
