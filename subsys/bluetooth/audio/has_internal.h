/** @file
 *  @brief Internal APIs for Bluetooth Hearing Access Profile.
 */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_HAS_FEAT_HEARING_AID_TYPE_MASK 0x03

struct bt_has {
	/** Hearing Aid Features value */
	uint8_t features;
};
