/*
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_SIH_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_SIH_
/**
 * @brief Private Set Unique identifier hash function sih.
 *
 * The PSRI hash function sih is used to generate a hash value that is
 * used in PSRIs - Used by the Coordinated Set Identification service and
 * profile.
 *
 * @param sirk The 16-byte SIRK
 * @param r 3 byte random value
 * @param out The 3 byte output buffer
 * @return int 0 on success, any other value indicates a failure. s
 */
int sih(const uint8_t sirk[16], const uint32_t r, uint32_t *out);
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_SIH_ */
