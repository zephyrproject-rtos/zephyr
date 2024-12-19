/* ieee802154_mcxw_utils.h - NXP MCXW7X 802.15.4 driver utils*/

/*
 * Copyright (c) NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

bool IsVersion2015(uint8_t *pdu, uint16_t lenght);

bool IsKeyIdMode1(uint8_t *pdu, uint16_t lenght);

void SetFrameCounter(uint8_t *pdu, uint16_t lenght, uint32_t fc);

void SetCslIe(uint8_t *pdu, uint16_t lenght, uint16_t period, uint16_t phase);