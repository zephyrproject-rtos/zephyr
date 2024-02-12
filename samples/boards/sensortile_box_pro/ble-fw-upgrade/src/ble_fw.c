/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

const uint8_t ble_fw_img[] = {
/* if not enabled do nothing, just remove blob dependency */
#ifdef CONFIG_BLOB_ENABLED
#include <dtm.bin.inc>
#endif
};

const uint32_t ble_fw_img_len = sizeof(ble_fw_img);
