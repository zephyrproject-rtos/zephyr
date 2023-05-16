/*
 * Copyright (c) 2023 Silpion IT Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <CANopen.h>
#include <303/crc16-ccitt.h>
#include <canopennode.h>


#if ((CO_CONFIG_CRC16) & CO_CONFIG_CRC16_EXTERNAL)
void crc16_ccitt_single(uint16_t *crc, const uint8_t chr) {
    uint16_t tmp = *crc;
    *crc = crc16_ccitt(&chr, 1, tmp);
}
#endif /* !((CO_CONFIG_CRC16) & CO_CONFIG_CRC16_EXTERNAL) */
