/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCM_SOC_PINS_H_
#define _NUVOTON_NPCM_SOC_PINS_H_

#include <stdint.h>

#include "reg/reg_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NPCM pin-mux configuration structure
 *
 * Used to indicate the device's corresponding DEVALT register/bit for
 * pin-muxing and its polarity to enable alternative functionality.
 */
struct npcm_alt {
	uint8_t group;
	uint8_t bit:3;
	uint8_t inverted:1;
	uint8_t reserved:4;
};

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCM_SOC_PINS_H_ */
