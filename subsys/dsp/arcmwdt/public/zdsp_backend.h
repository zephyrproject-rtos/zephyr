/*
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_MATH_ARC_BACKEND_PUBLIC_ZDSP_BACKEND_DSP_H_
#define SUBSYS_MATH_ARC_BACKEND_PUBLIC_ZDSP_BACKEND_DSP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* This include MUST be done before arm_math.h so we can let the arch specific
 * logic set up the right #define values for arm_math.h
 */
#include <zephyr/kernel.h>

#include <arm_math.h>
#include "dsplib.h"

static inline void zdsp_mult_q7(const DSP_DATA q7_t *src_a, const DSP_DATA q7_t *src_b,
				DSP_DATA q7_t *dst, uint32_t block_size)
{
	dsp_mult_q7(src_a, src_b, dst, block_size);
}
static inline void zdsp_mult_q15(const DSP_DATA q15_t *src_a, const DSP_DATA q15_t *src_b,
				DSP_DATA q15_t *dst, uint32_t block_size)
{
	dsp_mult_q15(src_a, src_b, dst, block_size);
}
static inline void zdsp_mult_q31(const DSP_DATA q31_t *src_a, const DSP_DATA q31_t *src_b,
				DSP_DATA q31_t *dst, uint32_t block_size)
{
	dsp_mult_q31(src_a, src_b, dst, block_size);
}
static inline void zdsp_mult_f32(const DSP_DATA float32_t *src_a, const DSP_DATA float32_t *src_b,
				DSP_DATA float32_t *dst, uint32_t block_size)
{
	dsp_mult_f32(src_a, src_b, dst, block_size);
}

static inline void zdsp_add_q7(const DSP_DATA q7_t *src_a, const DSP_DATA q7_t *src_b,
				DSP_DATA q7_t *dst, uint32_t block_size)
{
	dsp_add_q7(src_a, src_b, dst, block_size);
}
static inline void zdsp_add_q15(const DSP_DATA q15_t *src_a, const DSP_DATA q15_t *src_b,
				DSP_DATA q15_t *dst, uint32_t block_size)
{
	dsp_add_q15(src_a, src_b, dst, block_size);
}
static inline void zdsp_add_q31(const DSP_DATA q31_t *src_a, const DSP_DATA q31_t *src_b,
				DSP_DATA q31_t *dst, uint32_t block_size)
{
	dsp_add_q31(src_a, src_b, dst, block_size);
}
static inline void zdsp_add_f32(const DSP_DATA float32_t *src_a, const DSP_DATA float32_t *src_b,
				DSP_DATA float32_t *dst, uint32_t block_size)
{
	dsp_add_f32(src_a, src_b, dst, block_size);
}

static inline void zdsp_sub_q7(const DSP_DATA q7_t *src_a, const DSP_DATA q7_t *src_b,
				DSP_DATA q7_t *dst, uint32_t block_size)
{
	dsp_sub_q7(src_a, src_b, dst, block_size);
}
static inline void zdsp_sub_q15(const DSP_DATA q15_t *src_a, const DSP_DATA q15_t *src_b,
				DSP_DATA q15_t *dst, uint32_t block_size)
{
	dsp_sub_q15(src_a, src_b, dst, block_size);
}
static inline void zdsp_sub_q31(const DSP_DATA q31_t *src_a, const DSP_DATA q31_t *src_b,
				DSP_DATA q31_t *dst, uint32_t block_size)
{
	dsp_sub_q31(src_a, src_b, dst, block_size);
}
static inline void zdsp_sub_f32(const DSP_DATA float32_t *src_a, const DSP_DATA float32_t *src_b,
				DSP_DATA float32_t *dst, uint32_t block_size)
{
	dsp_sub_f32(src_a, src_b, dst, block_size);
}

static inline void zdsp_scale_q7(const DSP_DATA q7_t *src, q7_t scale_fract, int8_t shift,
				DSP_DATA q7_t *dst, uint32_t block_size)
{
	dsp_scale_q7(src, scale_fract, shift, dst, block_size);
}
static inline void zdsp_scale_q15(const DSP_DATA q15_t *src, q15_t scale_fract, int8_t shift,
				DSP_DATA q15_t *dst, uint32_t block_size)
{
	dsp_scale_q15(src, scale_fract, shift, dst, block_size);
}
static inline void zdsp_scale_q31(const DSP_DATA q31_t *src, q31_t scale_fract, int8_t shift,
				DSP_DATA q31_t *dst, uint32_t block_size)
{
	dsp_scale_q31(src, scale_fract, shift, dst, block_size);
}

static inline void zdsp_scale_f32(const DSP_DATA float32_t *src, float32_t scale,
				DSP_DATA float32_t *dst, uint32_t block_size)
{
	dsp_scale_f32(src, scale, dst, block_size);
}

static inline void zdsp_abs_q7(const DSP_DATA q7_t *src, DSP_DATA q7_t *dst, uint32_t block_size)
{
	dsp_abs_q7(src, dst, block_size);
}
static inline void zdsp_abs_q15(const DSP_DATA q15_t *src, DSP_DATA q15_t *dst, uint32_t block_size)
{
	dsp_abs_q15(src, dst, block_size);
}
static inline void zdsp_abs_q31(const DSP_DATA q31_t *src, DSP_DATA q31_t *dst, uint32_t block_size)
{
	dsp_abs_q31(src, dst, block_size);
}
static inline void zdsp_abs_f32(const DSP_DATA float32_t *src, DSP_DATA float32_t *dst,
				uint32_t block_size)
{
	dsp_abs_f32(src, dst, block_size);
}

static inline void zdsp_negate_q7(const DSP_DATA q7_t *src, DSP_DATA q7_t *dst, uint32_t block_size)
{
	dsp_negate_q7(src, dst, block_size);
}
static inline void zdsp_negate_q15(const DSP_DATA q15_t *src, DSP_DATA q15_t *dst,
				uint32_t block_size)
{
	dsp_negate_q15(src, dst, block_size);
}
static inline void zdsp_negate_q31(const DSP_DATA q31_t *src, DSP_DATA q31_t *dst,
				uint32_t block_size)
{
	dsp_negate_q31(src, dst, block_size);
}
static inline void zdsp_negate_f32(const DSP_DATA float32_t *src, DSP_DATA float32_t *dst,
				uint32_t block_size)
{
	dsp_negate_f32(src, dst, block_size);
}

static inline void zdsp_dot_prod_q7(const DSP_DATA q7_t *src_a, const DSP_DATA q7_t *src_b,
				uint32_t block_size, DSP_DATA q31_t *dst)
{
	dsp_dot_prod_q7(src_a, src_b, block_size, dst);
}
static inline void zdsp_dot_prod_q15(const DSP_DATA q15_t *src_a, const DSP_DATA q15_t *src_b,
				uint32_t block_size, DSP_DATA q63_t *dst)
{
	dsp_dot_prod_q15(src_a, src_b, block_size, dst);
}
static inline void zdsp_dot_prod_q31(const DSP_DATA q31_t *src_a, const DSP_DATA q31_t *src_b,
				uint32_t block_size, DSP_DATA q63_t *dst)
{
	dsp_dot_prod_q31(src_a, src_b, block_size, dst);
}
static inline void zdsp_dot_prod_f32(const DSP_DATA float32_t *src_a,
				const DSP_DATA float32_t *src_b, uint32_t block_size,
				DSP_DATA float32_t *dst)
{
	dsp_dot_prod_f32(src_a, src_b, block_size, dst);
}

static inline void zdsp_shift_q7(const DSP_DATA q7_t *src, int8_t shift_bits, DSP_DATA q7_t *dst,
				uint32_t block_size)
{
	dsp_shift_q7(src, shift_bits, dst, block_size);
}
static inline void zdsp_shift_q15(const DSP_DATA q15_t *src, int8_t shift_bits, DSP_DATA q15_t *dst,
				uint32_t block_size)
{
	dsp_shift_q15(src, shift_bits, dst, block_size);
}
static inline void zdsp_shift_q31(const DSP_DATA q31_t *src, int8_t shift_bits, DSP_DATA q31_t *dst,
				uint32_t block_size)
{
	dsp_shift_q31(src, shift_bits, dst, block_size);
}

static inline void zdsp_offset_q7(const DSP_DATA q7_t *src, q7_t offset, DSP_DATA q7_t *dst,
				uint32_t block_size)
{
	dsp_offset_q7(src, offset, dst, block_size);
}
static inline void zdsp_offset_q15(const DSP_DATA q15_t *src, q15_t offset, DSP_DATA q15_t *dst,
				uint32_t block_size)
{
	dsp_offset_q15(src, offset, dst, block_size);
}
static inline void zdsp_offset_q31(const DSP_DATA q31_t *src, q31_t offset, DSP_DATA q31_t *dst,
				uint32_t block_size)
{
	dsp_offset_q31(src, offset, dst, block_size);
}
static inline void zdsp_offset_f32(const DSP_DATA float32_t *src, float32_t offset,
				DSP_DATA float32_t *dst, uint32_t block_size)
{
	dsp_offset_f32(src, offset, dst, block_size);
}

static inline void zdsp_clip_q7(const DSP_DATA q7_t *src, DSP_DATA q7_t *dst, q7_t low, q7_t high,
				uint32_t num_samples)
{
	arm_clip_q7(src, dst, low, high, num_samples);
}
static inline void zdsp_clip_q15(const DSP_DATA q15_t *src, DSP_DATA q15_t *dst, q15_t low,
				q15_t high, uint32_t num_samples)
{
	arm_clip_q15(src, dst, low, high, num_samples);
}
static inline void zdsp_clip_q31(const DSP_DATA q31_t *src, DSP_DATA q31_t *dst, q31_t low,
				q31_t high, uint32_t num_samples)
{
	arm_clip_q31(src, dst, low, high, num_samples);
}
static inline void zdsp_clip_f32(const DSP_DATA float32_t *src, DSP_DATA float32_t *dst,
				float32_t low, float32_t high, uint32_t num_samples)
{
	arm_clip_f32(src, dst, low, high, num_samples);
}

static inline void zdsp_and_u8(const DSP_DATA uint8_t *src_a, const DSP_DATA uint8_t *src_b,
				DSP_DATA uint8_t *dst, uint32_t block_size)
{
	arm_and_u8(src_a, src_b, dst, block_size);
}
static inline void zdsp_and_u16(const DSP_DATA uint16_t *src_a, const DSP_DATA uint16_t *src_b,
				DSP_DATA uint16_t *dst, uint32_t block_size)
{
	arm_and_u16(src_a, src_b, dst, block_size);
}
static inline void zdsp_and_u32(const DSP_DATA uint32_t *src_a, const DSP_DATA uint32_t *src_b,
				DSP_DATA uint32_t *dst, uint32_t block_size)
{
	arm_and_u32(src_a, src_b, dst, block_size);
}

static inline void zdsp_or_u8(const DSP_DATA uint8_t *src_a, const DSP_DATA uint8_t *src_b,
				DSP_DATA uint8_t *dst, uint32_t block_size)
{
	arm_or_u8(src_a, src_b, dst, block_size);
}
static inline void zdsp_or_u16(const DSP_DATA uint16_t *src_a, const DSP_DATA uint16_t *src_b,
				DSP_DATA uint16_t *dst, uint32_t block_size)
{
	arm_or_u16(src_a, src_b, dst, block_size);
}
static inline void zdsp_or_u32(const DSP_DATA uint32_t *src_a, const DSP_DATA uint32_t *src_b,
				DSP_DATA uint32_t *dst, uint32_t block_size)
{
	arm_or_u32(src_a, src_b, dst, block_size);
}

static inline void zdsp_xor_u8(const DSP_DATA uint8_t *src_a, const DSP_DATA uint8_t *src_b,
				DSP_DATA uint8_t *dst, uint32_t block_size)
{
	arm_xor_u8(src_a, src_b, dst, block_size);
}
static inline void zdsp_xor_u16(const DSP_DATA uint16_t *src_a, const DSP_DATA uint16_t *src_b,
				DSP_DATA uint16_t *dst, uint32_t block_size)
{
	arm_xor_u16(src_a, src_b, dst, block_size);
}
static inline void zdsp_xor_u32(const DSP_DATA uint32_t *src_a, const DSP_DATA uint32_t *src_b,
				DSP_DATA uint32_t *dst, uint32_t block_size)
{
	arm_xor_u32(src_a, src_b, dst, block_size);
}

static inline void zdsp_not_u8(const DSP_DATA uint8_t *src, DSP_DATA uint8_t *dst,
				uint32_t block_size)
{
	arm_not_u8(src, dst, block_size);
}
static inline void zdsp_not_u16(const DSP_DATA uint16_t *src, DSP_DATA uint16_t *dst,
				uint32_t block_size)
{
	arm_not_u16(src, dst, block_size);
}
static inline void zdsp_not_u32(const DSP_DATA uint32_t *src, DSP_DATA uint32_t *dst,
				uint32_t block_size)
{
	arm_not_u32(src, dst, block_size);
}

/*
 * Transform (FFT) functions.
 *
 * These wrap the CMSIS-DSP transform routines (CONFIG_CMSIS_DSP is enabled for the ARC
 * backend), so the buffers use plain pointers rather than the DSP_DATA/AGU qualifiers used
 * by the MWDT basicmath routines above. The instance structures wrap the CMSIS-DSP
 * instances so the public zdsp API never exposes a CMSIS type.
 */
struct zdsp_cfft_instance_q15 {
	arm_cfft_instance_q15 arm;
};

static inline zdsp_transform_status zdsp_cfft_init_q15(struct zdsp_cfft_instance_q15 *inst,
						       uint16_t fft_len)
{
	return arm_cfft_init_q15(&inst->arm, fft_len) == ARM_MATH_SUCCESS
		       ? ZDSP_TRANSFORM_STATUS_OK
		       : ZDSP_TRANSFORM_STATUS_ERROR;
}
static inline void zdsp_cfft_q15(const struct zdsp_cfft_instance_q15 *inst, q15_t *p,
				 uint8_t ifft_flag, uint8_t bit_reverse_flag)
{
	arm_cfft_q15(&inst->arm, p, ifft_flag, bit_reverse_flag);
}

struct zdsp_cfft_instance_q31 {
	arm_cfft_instance_q31 arm;
};

static inline zdsp_transform_status zdsp_cfft_init_q31(struct zdsp_cfft_instance_q31 *inst,
						       uint16_t fft_len)
{
	return arm_cfft_init_q31(&inst->arm, fft_len) == ARM_MATH_SUCCESS
		       ? ZDSP_TRANSFORM_STATUS_OK
		       : ZDSP_TRANSFORM_STATUS_ERROR;
}
static inline void zdsp_cfft_q31(const struct zdsp_cfft_instance_q31 *inst, q31_t *p,
				 uint8_t ifft_flag, uint8_t bit_reverse_flag)
{
	arm_cfft_q31(&inst->arm, p, ifft_flag, bit_reverse_flag);
}

struct zdsp_cfft_instance_f32 {
	arm_cfft_instance_f32 arm;
};

static inline zdsp_transform_status zdsp_cfft_init_f32(struct zdsp_cfft_instance_f32 *inst,
						       uint16_t fft_len)
{
	return arm_cfft_init_f32(&inst->arm, fft_len) == ARM_MATH_SUCCESS
		       ? ZDSP_TRANSFORM_STATUS_OK
		       : ZDSP_TRANSFORM_STATUS_ERROR;
}
static inline void zdsp_cfft_f32(const struct zdsp_cfft_instance_f32 *inst, float32_t *p,
				 uint8_t ifft_flag, uint8_t bit_reverse_flag)
{
	arm_cfft_f32(&inst->arm, p, ifft_flag, bit_reverse_flag);
}

struct zdsp_rfft_instance_q15 {
	arm_rfft_instance_q15 arm;
};

static inline zdsp_transform_status zdsp_rfft_init_q15(struct zdsp_rfft_instance_q15 *inst,
						       uint32_t fft_len, uint32_t ifft_flag,
						       uint32_t bit_reverse_flag)
{
	return arm_rfft_init_q15(&inst->arm, fft_len, ifft_flag, bit_reverse_flag) ==
			       ARM_MATH_SUCCESS
		       ? ZDSP_TRANSFORM_STATUS_OK
		       : ZDSP_TRANSFORM_STATUS_ERROR;
}
static inline void zdsp_rfft_q15(const struct zdsp_rfft_instance_q15 *inst, q15_t *src, q15_t *dst)
{
	arm_rfft_q15(&inst->arm, src, dst);
}

struct zdsp_rfft_instance_q31 {
	arm_rfft_instance_q31 arm;
};

static inline zdsp_transform_status zdsp_rfft_init_q31(struct zdsp_rfft_instance_q31 *inst,
						       uint32_t fft_len, uint32_t ifft_flag,
						       uint32_t bit_reverse_flag)
{
	return arm_rfft_init_q31(&inst->arm, fft_len, ifft_flag, bit_reverse_flag) ==
			       ARM_MATH_SUCCESS
		       ? ZDSP_TRANSFORM_STATUS_OK
		       : ZDSP_TRANSFORM_STATUS_ERROR;
}
static inline void zdsp_rfft_q31(const struct zdsp_rfft_instance_q31 *inst, q31_t *src, q31_t *dst)
{
	arm_rfft_q31(&inst->arm, src, dst);
}

struct zdsp_rfft_fast_instance_f32 {
	arm_rfft_fast_instance_f32 arm;
};

static inline zdsp_transform_status
zdsp_rfft_fast_init_f32(struct zdsp_rfft_fast_instance_f32 *inst, uint16_t fft_len)
{
	return arm_rfft_fast_init_f32(&inst->arm, fft_len) == ARM_MATH_SUCCESS
		       ? ZDSP_TRANSFORM_STATUS_OK
		       : ZDSP_TRANSFORM_STATUS_ERROR;
}
static inline void zdsp_rfft_fast_f32(const struct zdsp_rfft_fast_instance_f32 *inst, float32_t *p,
				      float32_t *out, uint8_t ifft_flag)
{
	arm_rfft_fast_f32(&inst->arm, p, out, ifft_flag);
}

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_MATH_ARC_BACKEND_PUBLIC_ZDSP_BACKEND_DSP_H_ */
