/*
 * Copyright (c) 2024 A Labs GmbH
 * Copyright (c) 2024 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_SUBSYS_LORAWAN_FRAG_DECODER_SRC_FRAG_ENCODER_H_
#define TEST_SUBSYS_LORAWAN_FRAG_DECODER_SRC_FRAG_ENCODER_H_

#include <stdint.h>
#include <string.h>

/**
 * Generate coded binary data according to LoRaWAN TS004-1.0.0
 *
 * @param uncoded Pointer to uncoded data buffer (e.g. firmware binary)
 * @param uncoded_len Length of uncoded data in bytes
 * @param coded Pointer to buffer for resulting coded data
 * @param coded_size Size of the buffer for coded data
 * @param frag_size Fragment size to be used
 * @param redundant_frags Absolute number of redundant fragments to be generated
 *
 * @returns 0 for success or negative error code otherwise.
 */
int lorawan_frag_encoder(const uint8_t *uncoded, size_t uncoded_len, uint8_t *coded,
			 size_t coded_size, size_t frag_size, unsigned int redundant_frags);

#endif /* TEST_SUBSYS_LORAWAN_FRAG_DECODER_SRC_FRAG_ENCODER_H_ */
