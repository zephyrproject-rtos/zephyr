/*
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MISC_TI_K3_ESM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MISC_TI_K3_ESM_H_

/**
 * @file
 * @brief Event numbers for the TI K3 Error Signaling Module (ESM).
 *
 * Values are the ESM level/pulse input indices from the SoC TRM event map.
 * WKUP_ESM0 on J722S / AM67 (SPRUJB3D §4.15.2 / Table 6-xxx).
 */

/* MCU_DCC0 / MCU_DCC1 error outputs into WKUP_ESM0 */
#define K3_WKUP_ESM_EVT_MCU_DCC1_ERR 36
#define K3_WKUP_ESM_EVT_MCU_DCC0_ERR 37

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MISC_TI_K3_ESM_H_ */
