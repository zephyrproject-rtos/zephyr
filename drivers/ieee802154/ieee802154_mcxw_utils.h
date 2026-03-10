/* ieee802154_mcxw_utils.h - NXP MCXW 802.15.4 driver utils*/

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

bool is_frame_version_2015(uint8_t *pdu, uint16_t length);

bool is_keyid_mode_1(uint8_t *pdu, uint16_t length);

void set_frame_counter(uint8_t *pdu, uint16_t length, uint32_t fc);

void set_csl_ie(uint8_t *pdu, uint16_t length, uint16_t period, uint16_t phase);
