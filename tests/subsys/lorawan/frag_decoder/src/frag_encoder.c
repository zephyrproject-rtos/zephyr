/*
 * Copyright (c) 2024 A Labs GmbH
 * Copyright (c) 2024 tado GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Implementation of the fragment encoding algorithm described in the LoRaWAN TS004-1.0.0.
 * https://lora-alliance.org/wp-content/uploads/2020/11/fragmented_data_block_transport_v1.0.0.pdf
 *
 * Note: This algorithm is not compatible with TS004-2.0.0, which has some subtle differences
 *       in the parity matrix generation.
 *
 * Variable naming according to LoRaWAN specification:
 *
 * M: Number of uncoded fragments (original data)
 * N: Number of coded fragments (including the original data at the beginning)
 * CR: Coding ratio M/N
 */

#include "frag_encoder.h"

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lorawan_frag_enc, CONFIG_LORAWAN_SERVICES_LOG_LEVEL);

/**
 * Generate a 23bit Pseudorandom Binary Sequence (PRBS)
 *
 * @param seed Seed input value
 *
 * @returns Pseudorandom output value
 */
static int32_t prbs23(int32_t seed)
{
	int32_t b0 = seed & 1;
	int32_t b1 = (seed & 32) / 32;

	return (seed / 2) + ((b0 ^ b1) << 22);
}

/**
 * Generate vector for coded fragment n of the MxN parity matrix
 *
 * @param m Total number of uncoded fragments (M)
 * @param n Coded fragment number (starting at 1 and not 0)
 * @param vec Output vector (buffer size must be greater than m)
 */
void lorawan_fec_parity_matrix_vector(int m, int n, uint8_t *vec)
{
	int mm, x, r;

	memset(vec, 0, m);

	/*
	 * Powers of 2 must be treated differently to make sure matrix content is close
	 * to random. Powers of 2 tend to generate patterns.
	 */
	if (is_power_of_two(m)) {
		mm = m + 1;
	} else {
		mm = m;
	}

	x = 1 + (1001 * n);

	for (int nb_coeff = 0; nb_coeff < (m / 2); nb_coeff++) {
		r = (1 << 16);
		while (r >= m) {
			x = prbs23(x);
			r = x % mm;
		}
		vec[r] = 1;
	}
}

int lorawan_frag_encoder(const uint8_t *uncoded, size_t uncoded_len, uint8_t *coded,
			 size_t coded_size, size_t frag_size, unsigned int redundant_frags)
{
	int uncoded_frags = DIV_ROUND_UP(uncoded_len, frag_size);
	int coded_frags = uncoded_frags + redundant_frags;
	uint8_t parity_vec[frag_size];

	memset(parity_vec, 0, sizeof(parity_vec));

	if (coded_size < coded_frags * frag_size) {
		LOG_ERR("output buffer not large enough");
		return -EINVAL;
	}

	/* copy uncoded frags to the beginning of coded fragments and pad with zeros */
	memcpy(coded, uncoded, uncoded_len);
	memset(coded + uncoded_len, 0, uncoded_frags * frag_size - uncoded_len);

	/* generate remaining coded (redundant) frags */
	for (int i = 1; i <= redundant_frags; i++) {
		lorawan_fec_parity_matrix_vector(uncoded_frags, i, parity_vec);

		uint8_t *out = coded + (uncoded_frags + i - 1) * frag_size;

		for (int j = 0; j < uncoded_frags; j++) {
			if (parity_vec[j] == 1) {
				for (int m = 0; m < frag_size; m++) {
					out[m] ^= coded[j * frag_size + m];
				}
			}
		}
	}

	return 0;
}
