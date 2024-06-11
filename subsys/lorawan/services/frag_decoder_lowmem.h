/*
 * Copyright (c) 2024 A Labs GmbH
 * Copyright (c) 2024 tado GmbH
 * Copyright (c) 2022 Jiapeng Li
 *
 * Based on: https://github.com/JiapengLi/LoRaWANFragmentedDataBlockTransportAlgorithm
 * Original algorithm: http://www.inference.org.uk/mackay/gallager/papers/ldpc.pdf
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAG_DEC_H_
#define FRAG_DEC_H_

#include <stdint.h>
#include <stddef.h>

#define FRAG_MAX_NB                                                                                \
	(CONFIG_LORAWAN_FRAG_TRANSPORT_IMAGE_SIZE / CONFIG_LORAWAN_FRAG_TRANSPORT_MIN_FRAG_SIZE +  \
	 1U)
#define FRAG_MAX_SIZE  (CONFIG_LORAWAN_FRAG_TRANSPORT_MAX_FRAG_SIZE)
#define FRAG_TOLERANCE (FRAG_MAX_NB * CONFIG_LORAWAN_FRAG_TRANSPORT_MAX_REDUNDANCY / 100U)

#define FRAG_DEC_ONGOING                  (-1)
#define FRAG_DEC_ERR_INVALID_FRAME        (-2)
#define FRAG_DEC_ERR_TOO_MANY_FRAMES_LOST (-3)
#define FRAG_DEC_ERR                      (-4)

enum frag_decoder_status {
	/** Processing uncoded fragments */
	FRAG_DEC_STA_UNCODED,
	/** Processing coded fragments and restoring data with the help of other fragments */
	FRAG_DEC_STA_CODED,
	/** All fragments received and/or restored */
	FRAG_DEC_STA_DONE,
};

struct frag_decoder {
	/** Current decoder status */
	enum frag_decoder_status status;

	/** Number of fragments */
	uint16_t nb_frag;
	/** Size of individual fragment */
	uint8_t frag_size;

	/** Number of frames lost in this session */
	uint16_t lost_frame_count;
	/** Number of frames recovered in this session */
	uint16_t filled_lost_frame_count;
};

void frag_dec_init(struct frag_decoder *decoder, size_t nb_frag, size_t frag_size);
int frag_dec(struct frag_decoder *decoder, uint16_t frag_counter, const uint8_t *buf, size_t len);

#endif /* FRAG_DEC_H_ */
