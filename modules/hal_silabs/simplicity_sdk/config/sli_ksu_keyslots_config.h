/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This configuration header is used by the HAL driver sli_crypto_ksu_manager
 * from hal_silabs when host crypto is enabled on Series 3.
 */

#ifndef SLI_KSU_KEYSLOTS_CONFIG_H
#define SLI_KSU_KEYSLOTS_CONFIG_H

#include <soc.h>

#define SLI_KSU_MAX_KEY_SLOTS       KSU_MAX_KEY_SLOTS
#define SLI_KSU_KEY_SLOT_USER_START 1

#endif
