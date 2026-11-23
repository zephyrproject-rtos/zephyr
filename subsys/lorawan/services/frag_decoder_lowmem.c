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

#include "frag_decoder_lowmem.h"
#include "frag_flash.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/bitarray.h>

LOG_MODULE_REGISTER(lorawan_frag_dec, CONFIG_LORAWAN_SERVICES_LOG_LEVEL);

SYS_BITARRAY_DEFINE_STATIC(lost_frames, FRAG_MAX_NB);
SYS_BITARRAY_DEFINE_STATIC(lost_frames_matrix, (FRAG_TOLERANCE * (FRAG_TOLERANCE + 1) / 2));
SYS_BITARRAY_DEFINE_STATIC(matched_lost_frm_bm0, FRAG_TOLERANCE);
SYS_BITARRAY_DEFINE_STATIC(matched_lost_frm_bm1, FRAG_TOLERANCE);
SYS_BITARRAY_DEFINE_STATIC(matrix_line_bm, FRAG_MAX_NB);


static inline size_t matrix_location_to_index(size_t x, size_t y, size_t m)
{
	/* We only store the top half of the matrix because it is triangular,
	 * but that means when mapping the coordinates into the flat representation
	 * we need to account for that
	 */
	return (y + 1) * (m + m - y) / 2 - (m - x);
}

static bool triangular_matrix_get_entry(struct sys_bitarray *m2tbm, size_t x, size_t y, size_t m)
{
	/* We are dealing with triangular matrices, so we don't expect actions in the lower half */
	__ASSERT(x >= y, "x: %d, y: %d, m: %d", x, y, m);
	size_t bit;
	int ret;

	ret = sys_bitarray_test_bit(m2tbm, matrix_location_to_index(x, y, m), &bit);
	__ASSERT_NO_MSG(ret == 0);

	return bit != 0;
}

static void triangular_matrix_set_entry(struct sys_bitarray *m2tbm, size_t x, size_t y, size_t m)
{
	/* We are dealing with triangular matrices, so we don't expect actions in the lower half */
	__ASSERT(x >= y, "x: %d, y: %d, m: %d", x, y, m);
	int ret;

	ret = sys_bitarray_set_bit(m2tbm, matrix_location_to_index(x, y, m));
	__ASSERT_NO_MSG(ret == 0);
}

static void triangular_matrix_clear_entry(struct sys_bitarray *m2tbm, size_t x, size_t y, size_t m)
{
	/* We are dealing with triangular matrices, so we don't expect actions in the lower half */
	__ASSERT(x >= y, "x: %d, y: %d, m: %d", x, y, m);

	int ret;

	ret = sys_bitarray_clear_bit(m2tbm, matrix_location_to_index(x, y, m));
	__ASSERT_NO_MSG(ret == 0);
}

static inline bool bit_get(struct sys_bitarray *bitmap, size_t index)
{
	int bit, ret;

	ret = sys_bitarray_test_bit(bitmap, index, &bit);
	__ASSERT_NO_MSG(ret == 0);
	return bit != 0;
}

static inline void bit_set(struct sys_bitarray *bitmap, size_t index)
{
	int ret;

	ret = sys_bitarray_set_bit(bitmap, index);
	__ASSERT_NO_MSG(ret == 0);
}

static inline void bit_clear(struct sys_bitarray *bitmap, size_t index)
{
	int ret;

	ret = sys_bitarray_clear_bit(bitmap, index);
	__ASSERT_NO_MSG(ret == 0);
}

static inline size_t bit_count_ones(struct sys_bitarray *bitmap, size_t index)
{
	size_t count;
	int ret;

	ret = sys_bitarray_popcount_region(bitmap, index + 1, 0, &count);
	__ASSERT_NO_MSG(ret == 0);
	return count;
}

static inline void bit_xor(struct sys_bitarray *des, struct sys_bitarray *src, size_t size)
{
	int ret;

	ret = sys_bitarray_xor(des, src, size, 0);
	__ASSERT_NO_MSG(ret == 0);
}

static inline void bit_clear_all(struct sys_bitarray *bitmap, size_t size)
{
	int ret;

	ret = sys_bitarray_clear_region(bitmap, size, 0);
	__ASSERT_NO_MSG(ret == 0);
}

/**
 * Generate a 23bit Pseudorandom Binary Sequence (PRBS)
 *
 * @param previous Previous value in the sequence
 *
 * @returns Next value in the pseudorandom sequence
 */
static int32_t prbs23(int32_t previous)
{
	int32_t b0 = previous & 1;
	int32_t b1 = (previous & 32) / 32;

	return (previous / 2) + ((b0 ^ b1) << 22);
}

/**
 * Generate vector for coded fragment n of the MxN parity matrix
 *
 * @param m Total number of uncoded fragments (M)
 * @param n Coded fragment number (starting at 1 and not 0)
 * @param vec Output vector (buffer size must be greater than m)
 */
static void frag_dec_parity_matrix_vector(size_t m, size_t n, struct sys_bitarray *vec)
{
	size_t mm, r;
	int32_t x;
	int ret;

	ret = sys_bitarray_clear_region(vec, m, 0);
	__ASSERT_NO_MSG(ret == 0);

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

	for (size_t nb_coeff = 0; nb_coeff < (m / 2); nb_coeff++) {
		r = (1 << 16);
		while (r >= m) {
			x = prbs23(x);
			r = x % mm;
		}
		ret = sys_bitarray_set_bit(vec, r);
		__ASSERT_NO_MSG(ret == 0);
	}
}

void frag_dec_init(struct frag_decoder *decoder, size_t nb_frag, size_t frag_size)
{
	decoder->nb_frag = nb_frag;
	decoder->frag_size = frag_size;
	/* Set all frames lost, from 0 to nb_frag-1 */
	decoder->lost_frame_count = decoder->nb_frag;
	sys_bitarray_set_region(&lost_frames, decoder->nb_frag, 0);

	sys_bitarray_clear_region(&lost_frames_matrix, (FRAG_TOLERANCE * (FRAG_TOLERANCE + 1) / 2),
				  0);

	decoder->filled_lost_frame_count = 0;
	decoder->status = FRAG_DEC_STA_UNCODED;
}

void frag_dec_frame_received(struct frag_decoder *decoder, uint16_t index)
{
	int ret, was_set;

	ret = sys_bitarray_test_and_clear_bit(&lost_frames, index, &was_set);
	__ASSERT_NO_MSG(ret == 0);

	if (was_set != 0) {
		decoder->lost_frame_count--;
	}
}

static void frag_dec_write_vector(struct sys_bitarray *matrix, uint16_t line_index,
				  struct sys_bitarray *vector, size_t len)
{
	for (size_t i = line_index; i < len; i++) {
		if (bit_get(vector, i)) {
			triangular_matrix_set_entry(matrix, i, line_index, len);
		} else {
			triangular_matrix_clear_entry(matrix, i, line_index, len);
		}
	}
}

static void frag_dec_read_vector(struct sys_bitarray *matrix, uint16_t line_index,
				 struct sys_bitarray *vector, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (i >= line_index && triangular_matrix_get_entry(matrix, i, line_index, len)) {
			bit_set(vector, i);
		} else {
			bit_clear(vector, i);
		}
	}
}

int frag_dec(struct frag_decoder *decoder, uint16_t frag_counter, const uint8_t *buf, size_t len)
{
	int ret;
	int i, j;
	size_t unmatched_frame_count;
	size_t lost_frame_index, frame_index;
	static uint8_t row_data_buf[FRAG_MAX_SIZE];
	static uint8_t xor_row_data_buf[FRAG_MAX_SIZE];

	if (decoder->status == FRAG_DEC_STA_DONE) {
		return decoder->lost_frame_count;
	}

	if (len != decoder->frag_size) {
		return FRAG_DEC_ERR_INVALID_FRAME;
	}

	__ASSERT_NO_MSG(frag_counter > 0);

	if (frag_counter <= decoder->nb_frag && decoder->status == FRAG_DEC_STA_UNCODED) {
		/* Mark new received frame */
		frag_dec_frame_received(decoder, frag_counter - 1);
		/* Save data to flash */
		frag_flash_write((frag_counter - 1) * decoder->frag_size, (uint8_t *)buf,
				 decoder->frag_size);
		/* If no frame was lost, we are already done */
		if (decoder->lost_frame_count == 0) {
			decoder->status = FRAG_DEC_STA_DONE;
			return decoder->lost_frame_count;
		}
		return FRAG_DEC_ONGOING;
	}
	/* At least one frame was lost, start recovering frames */
	decoder->status = FRAG_DEC_STA_CODED;

	/* Clear all temporary bm and buf */
	bit_clear_all(&matched_lost_frm_bm0, decoder->lost_frame_count);
	bit_clear_all(&matched_lost_frm_bm1, decoder->lost_frame_count);

	/* Copy data buffer because we need to manipulate it */
	memcpy(xor_row_data_buf, buf, decoder->frag_size);

	if (decoder->lost_frame_count > FRAG_TOLERANCE) {
		return FRAG_DEC_ERR_TOO_MANY_FRAMES_LOST;
	}

	unmatched_frame_count = 0;
	/* Build parity matrix vector for current line */
	frag_dec_parity_matrix_vector(decoder->nb_frag, frag_counter - decoder->nb_frag,
				      &matrix_line_bm);
	for (i = 0; i < decoder->nb_frag; i++) {
		if (!bit_get(&matrix_line_bm, i)) {
			/* This frame is not part of the recovery matrix for the current fragment */
			continue;
		}
		if (bit_get(&lost_frames, i)) {
			/* No match for this coded frame in the uncoded frames.
			 * Check which lost frame we are processing by checking how many have been
			 * lost between the start and the current coded fragment.
			 */
			bit_set(&matched_lost_frm_bm0, bit_count_ones(&lost_frames, i) - 1);
			unmatched_frame_count++;
		} else {
			/* Restore frame by XORing with already received frame */
			/* Load previously received data into buffer */
			frag_flash_read(i * decoder->frag_size, row_data_buf, decoder->frag_size);
			/* XOR previously received data with data for current frame */
			mem_xor_n(xor_row_data_buf, xor_row_data_buf, row_data_buf,
				  decoder->frag_size);
		}
	}
	if (unmatched_frame_count == 0) {
		return FRAG_DEC_ONGOING;
	}

	/* &matched_lost_frm_bm0 now contains new coded frame which excludes all received
	 * frames content start to diagonal &matched_lost_frm_bm0
	 */
	do {
		ret = sys_bitarray_find_nth_set(&matched_lost_frm_bm0, 1, decoder->lost_frame_count,
						0, &lost_frame_index);
		if (ret == 1) {
			/* Not found */
			break;
		}
		if (ret != 0) {
			return FRAG_DEC_ERR;
		}
		/* We know which one is the next lost frame, try to find it in the lost frame bitmap
		 */
		ret = sys_bitarray_find_nth_set(&lost_frames, lost_frame_index + 1,
						decoder->nb_frag, 0, &frame_index);
		if (ret == 1) {
			/* Not found */
			break;
		}
		if (ret != 0) {
			return FRAG_DEC_ERR;
		}
		/* If current frame contains new information, save it */
		if (!triangular_matrix_get_entry(&lost_frames_matrix, lost_frame_index,
						 lost_frame_index, decoder->lost_frame_count)) {
			frag_dec_write_vector(&lost_frames_matrix, lost_frame_index,
					      &matched_lost_frm_bm0, decoder->lost_frame_count);
			frag_flash_write(frame_index * decoder->frag_size, xor_row_data_buf,
					 decoder->frag_size);
			decoder->filled_lost_frame_count++;
			break;
		}

		frag_dec_read_vector(&lost_frames_matrix, lost_frame_index, &matched_lost_frm_bm1,
				     decoder->lost_frame_count);
		bit_xor(&matched_lost_frm_bm0, &matched_lost_frm_bm1, decoder->lost_frame_count);
		frag_flash_read(frame_index * decoder->frag_size, row_data_buf, decoder->frag_size);
		mem_xor_n(xor_row_data_buf, xor_row_data_buf, row_data_buf, decoder->frag_size);
	} while (!sys_bitarray_is_region_cleared(&matched_lost_frm_bm0, decoder->lost_frame_count,
						 0));

	if (decoder->filled_lost_frame_count != decoder->lost_frame_count) {
		return FRAG_DEC_ONGOING;
	}

	if (decoder->lost_frame_count < 2) {
		decoder->status = FRAG_DEC_STA_DONE;
		return decoder->lost_frame_count;
	}

	/* All frame content is received, now to reconstruct the whole frame */
	for (i = (decoder->lost_frame_count - 2); i >= 0; i--) {
		ret = sys_bitarray_find_nth_set(&lost_frames, i + 1, decoder->nb_frag, 0,
						&frame_index);
		if (ret != 0) {
			return FRAG_DEC_ERR;
		}
		frag_flash_read(frame_index * decoder->frag_size, xor_row_data_buf,
				decoder->frag_size);
		frag_dec_read_vector(&lost_frames_matrix, i, &matched_lost_frm_bm1,
				     decoder->lost_frame_count);
		for (j = (decoder->lost_frame_count - 1); j > i; j--) {
			if (!bit_get(&matched_lost_frm_bm1, j)) {
				continue;
			}
			ret = sys_bitarray_find_nth_set(&lost_frames, j + 1, decoder->nb_frag, 0,
							&lost_frame_index);
			if (ret != 0) {
				return FRAG_DEC_ERR;
			}
			frag_dec_read_vector(&lost_frames_matrix, j, &matched_lost_frm_bm0,
					     decoder->lost_frame_count);
			bit_xor(&matched_lost_frm_bm1, &matched_lost_frm_bm0,
				decoder->lost_frame_count);
			frag_flash_read(lost_frame_index * decoder->frag_size, row_data_buf,
					decoder->frag_size);
			mem_xor_n(xor_row_data_buf, xor_row_data_buf, row_data_buf,
				  decoder->frag_size);
			frag_dec_write_vector(&lost_frames_matrix, i, &matched_lost_frm_bm1,
					      decoder->lost_frame_count);
		}
		frag_flash_write(frame_index * decoder->frag_size, xor_row_data_buf,
				 decoder->frag_size);
	}
	decoder->status = FRAG_DEC_STA_DONE;
	return decoder->lost_frame_count;
}
