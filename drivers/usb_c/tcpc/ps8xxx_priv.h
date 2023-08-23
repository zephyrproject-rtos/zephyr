/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

/* Note: The Product ID will read as 0x8803 if the firmware has malfunctioned in 8705, 8755 and
 * 8805. Also the Product ID can be invalid if chip is before or during the power-up and
 * initialization process.
 */
#define PS8705_PRODUCT_ID 0x8705
#define PS8745_PRODUCT_ID 0x8745
#define PS8751_PRODUCT_ID 0x8751
#define PS8755_PRODUCT_ID 0x8755
#define PS8805_PRODUCT_ID 0x8805
#define PS8815_PRODUCT_ID 0x8815

/** PS8815 only - vendor specific register for firmware version */
#define PS8815_REG_FW_VER 0x82
