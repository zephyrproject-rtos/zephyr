/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NCT_SOC_PINS_H_
#define _NUVOTON_NCT_SOC_PINS_H_

#include <stdint.h>

#include "reg/reg_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NCT pin-mux configuration structure
 *
 * Used to indicate the device's corresponding DEVALT register/bit for
 * pin-muxing and its polarity to enable alternative functionality.
 */
struct nct_alt {
	uint8_t group;
	uint8_t bit:3;
	uint8_t inverted:1;
	uint8_t reserved:4;
};

#define NCT_FIU_FLASH_WP	BIT(0)
#define NCT_SPIM_FLASH_WP	BIT(1)
#define NCT_SPIP_FLASH_WP	BIT(2)

int nct_pinctrl_flash_write_protect_set(int interface);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NCT_SOC_PINS_H_ */
