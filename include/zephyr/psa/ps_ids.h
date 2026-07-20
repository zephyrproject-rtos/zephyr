/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_PSA_PS_IDS_H_
#define ZEPHYR_INCLUDE_PSA_PS_IDS_H_

/** UID range to be used by the TF-M PSA PS Settings backend. */
#define ZEPHYR_PSA_SETTINGS_TFM_PS_UID_RANGE_BEGIN 0x2C000000
#define ZEPHYR_PSA_SETTINGS_TFM_PS_UID_RANGE_SIZE  0x10000 /* 64 Ki */

/** UID range to be used by the TLS credentials PS backend. */
#define ZEPHYR_PSA_TLS_CREDENTIALS_PS_UID_RANGE_BEGIN 0xC2E0000000000000
#define ZEPHYR_PSA_TLS_CREDENTIALS_PS_UID_RANGE_SIZE  0x0001000000000000 /**< 2^48 */

#endif /* ZEPHYR_INCLUDE_PSA_PS_IDS_H_ */
