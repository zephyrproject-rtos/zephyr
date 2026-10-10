/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_DSP_POWERQUAD_PUBLIC_ZDSP_BACKEND_H_
#define SUBSYS_DSP_POWERQUAD_PUBLIC_ZDSP_BACKEND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#include <arm_math.h>

/*
 * PowerQuad-accelerated implementations (defined in powerquad_dsp.c).
 */

/* Binary operations: add/sub/mult */
extern void pq_mult_f32(const float32_t *src_a, const float32_t *src_b,
			 float32_t *dst, uint32_t block_size);
extern void pq_mult_q31(const q31_t *src_a, const q31_t *src_b,
			 q31_t *dst, uint32_t block_size);
extern void pq_mult_q15(const q15_t *src_a, const q15_t *src_b,
			 q15_t *dst, uint32_t block_size);
extern void pq_add_f32(const float32_t *src_a, const float32_t *src_b,
			float32_t *dst, uint32_t block_size);
extern void pq_add_q31(const q31_t *src_a, const q31_t *src_b,
			q31_t *dst, uint32_t block_size);
extern void pq_add_q15(const q15_t *src_a, const q15_t *src_b,
			q15_t *dst, uint32_t block_size);
extern void pq_sub_f32(const float32_t *src_a, const float32_t *src_b,
			float32_t *dst, uint32_t block_size);
extern void pq_sub_q31(const q31_t *src_a, const q31_t *src_b,
			q31_t *dst, uint32_t block_size);
extern void pq_sub_q15(const q15_t *src_a, const q15_t *src_b,
			q15_t *dst, uint32_t block_size);

/* Scale operations */
extern void pq_scale_f32(const float32_t *src, float32_t scale,
			  float32_t *dst, uint32_t block_size);
extern void pq_scale_q31(const q31_t *src, q31_t scale_fract, int8_t shift,
			  q31_t *dst, uint32_t block_size);
extern void pq_scale_q15(const q15_t *src, q15_t scale_fract, int8_t shift,
			  q15_t *dst, uint32_t block_size);

/* Negate operations */
extern void pq_negate_f32(const float32_t *src, float32_t *dst, uint32_t block_size);
extern void pq_negate_q31(const q31_t *src, q31_t *dst, uint32_t block_size);
extern void pq_negate_q15(const q15_t *src, q15_t *dst, uint32_t block_size);

/* Dot product operations */
extern void pq_dot_prod_f32(const float32_t *src_a, const float32_t *src_b,
			     uint32_t block_size, float32_t *dst);

/* Multiply */
static inline void zdsp_mult_f32(const float32_t *src_a, const float32_t *src_b,
				  float32_t *dst, uint32_t block_size)
{
	pq_mult_f32(src_a, src_b, dst, block_size);
}
static inline void zdsp_mult_q31(const q31_t *src_a, const q31_t *src_b,
				  q31_t *dst, uint32_t block_size)
{
#ifdef CONFIG_DSP_BACKEND_POWERQUAD_SUPPORT_Q31
	pq_mult_q31(src_a, src_b, dst, block_size);
#else
	arm_mult_q31(src_a, src_b, dst, block_size);
#endif
}
static inline void zdsp_mult_q15(const q15_t *src_a, const q15_t *src_b,
				  q15_t *dst, uint32_t block_size)
{
	pq_mult_q15(src_a, src_b, dst, block_size);
}
static inline void zdsp_mult_q7(const q7_t *src_a, const q7_t *src_b, q7_t *dst,
				 uint32_t block_size)
{
	arm_mult_q7(src_a, src_b, dst, block_size);
}

/* Add */
static inline void zdsp_add_f32(const float32_t *src_a, const float32_t *src_b,
				 float32_t *dst, uint32_t block_size)
{
	pq_add_f32(src_a, src_b, dst, block_size);
}
static inline void zdsp_add_q31(const q31_t *src_a, const q31_t *src_b,
				 q31_t *dst, uint32_t block_size)
{
#ifdef CONFIG_DSP_BACKEND_POWERQUAD_SUPPORT_Q31
	pq_add_q31(src_a, src_b, dst, block_size);
#else
	arm_add_q31(src_a, src_b, dst, block_size);
#endif
}
static inline void zdsp_add_q15(const q15_t *src_a, const q15_t *src_b,
				 q15_t *dst, uint32_t block_size)
{
	pq_add_q15(src_a, src_b, dst, block_size);
}
static inline void zdsp_add_q7(const q7_t *src_a, const q7_t *src_b, q7_t *dst,
			       uint32_t block_size)
{
	arm_add_q7(src_a, src_b, dst, block_size);
}

/* Subtract */
static inline void zdsp_sub_f32(const float32_t *src_a, const float32_t *src_b,
				 float32_t *dst, uint32_t block_size)
{
	pq_sub_f32(src_a, src_b, dst, block_size);
}
static inline void zdsp_sub_q31(const q31_t *src_a, const q31_t *src_b,
				 q31_t *dst, uint32_t block_size)
{
#ifdef CONFIG_DSP_BACKEND_POWERQUAD_SUPPORT_Q31
	pq_sub_q31(src_a, src_b, dst, block_size);
#else
	arm_sub_q31(src_a, src_b, dst, block_size);
#endif
}
static inline void zdsp_sub_q15(const q15_t *src_a, const q15_t *src_b,
				 q15_t *dst, uint32_t block_size)
{
	pq_sub_q15(src_a, src_b, dst, block_size);
}
static inline void zdsp_sub_q7(const q7_t *src_a, const q7_t *src_b, q7_t *dst,
			       uint32_t block_size)
{
	arm_sub_q7(src_a, src_b, dst, block_size);
}

/* Scale */
static inline void zdsp_scale_f32(const float32_t *src, float32_t scale, float32_t *dst,
				   uint32_t block_size)
{
	pq_scale_f32(src, scale, dst, block_size);
}
static inline void zdsp_scale_q31(const q31_t *src, q31_t scale_fract, int8_t shift, q31_t *dst,
				   uint32_t block_size)
{
#ifdef CONFIG_DSP_BACKEND_POWERQUAD_SUPPORT_Q31
	pq_scale_q31(src, scale_fract, shift, dst, block_size);
#else
	arm_scale_q31(src, scale_fract, shift, dst, block_size);
#endif
}
static inline void zdsp_scale_q15(const q15_t *src, q15_t scale_fract, int8_t shift, q15_t *dst,
				   uint32_t block_size)
{
	pq_scale_q15(src, scale_fract, shift, dst, block_size);
}
static inline void zdsp_scale_q7(const q7_t *src, q7_t scale_fract, int8_t shift, q7_t *dst,
				  uint32_t block_size)
{
	arm_scale_q7(src, scale_fract, shift, dst, block_size);
}

/* Abs (CMSIS-DSP only) */
static inline void zdsp_abs_q7(const q7_t *src, q7_t *dst, uint32_t block_size)
{
	arm_abs_q7(src, dst, block_size);
}
static inline void zdsp_abs_q15(const q15_t *src, q15_t *dst, uint32_t block_size)
{
	arm_abs_q15(src, dst, block_size);
}
static inline void zdsp_abs_q31(const q31_t *src, q31_t *dst, uint32_t block_size)
{
	arm_abs_q31(src, dst, block_size);
}
static inline void zdsp_abs_f32(const float32_t *src, float32_t *dst, uint32_t block_size)
{
	arm_abs_f32(src, dst, block_size);
}

/* Negate */
static inline void zdsp_negate_f32(const float32_t *src, float32_t *dst, uint32_t block_size)
{
	pq_negate_f32(src, dst, block_size);
}
static inline void zdsp_negate_q31(const q31_t *src, q31_t *dst, uint32_t block_size)
{
#ifdef CONFIG_DSP_BACKEND_POWERQUAD_SUPPORT_Q31
	pq_negate_q31(src, dst, block_size);
#else
	arm_negate_q31(src, dst, block_size);
#endif
}
static inline void zdsp_negate_q15(const q15_t *src, q15_t *dst, uint32_t block_size)
{
	pq_negate_q15(src, dst, block_size);
}
static inline void zdsp_negate_q7(const q7_t *src, q7_t *dst, uint32_t block_size)
{
	arm_negate_q7(src, dst, block_size);
}

/* Dot product */
static inline void zdsp_dot_prod_f32(const float32_t *src_a, const float32_t *src_b,
				      uint32_t block_size, float32_t *dst)
{
	pq_dot_prod_f32(src_a, src_b, block_size, dst);
}
/*
 * Dot product q31/q15: use CMSIS-DSP directly (not PowerQuad).
 * PowerQuad computes internally in float32 (24-bit mantissa), which cannot
 * produce the q63 accumulator precision required by dot product.
 */
static inline void zdsp_dot_prod_q31(const q31_t *src_a, const q31_t *src_b,
				      uint32_t block_size, q63_t *dst)
{
	arm_dot_prod_q31(src_a, src_b, block_size, dst);
}

static inline void zdsp_dot_prod_q15(const q15_t *src_a, const q15_t *src_b,
				      uint32_t block_size, q63_t *dst)
{
	arm_dot_prod_q15(src_a, src_b, block_size, dst);
}
static inline void zdsp_dot_prod_q7(const q7_t *src_a, const q7_t *src_b, uint32_t block_size,
				     q31_t *dst)
{
	arm_dot_prod_q7(src_a, src_b, block_size, dst);
}

/* Shift (CMSIS-DSP only) */
static inline void zdsp_shift_q7(const q7_t *src, int8_t shift_bits, q7_t *dst,
				  uint32_t block_size)
{
	arm_shift_q7(src, shift_bits, dst, block_size);
}
static inline void zdsp_shift_q15(const q15_t *src, int8_t shift_bits, q15_t *dst,
				   uint32_t block_size)
{
	arm_shift_q15(src, shift_bits, dst, block_size);
}
static inline void zdsp_shift_q31(const q31_t *src, int8_t shift_bits, q31_t *dst,
				   uint32_t block_size)
{
	arm_shift_q31(src, shift_bits, dst, block_size);
}

/* Offset (CMSIS-DSP only) */
static inline void zdsp_offset_q7(const q7_t *src, q7_t offset, q7_t *dst, uint32_t block_size)
{
	arm_offset_q7(src, offset, dst, block_size);
}
static inline void zdsp_offset_q15(const q15_t *src, q15_t offset, q15_t *dst,
				    uint32_t block_size)
{
	arm_offset_q15(src, offset, dst, block_size);
}
static inline void zdsp_offset_q31(const q31_t *src, q31_t offset, q31_t *dst,
				    uint32_t block_size)
{
	arm_offset_q31(src, offset, dst, block_size);
}
static inline void zdsp_offset_f32(const float32_t *src, float32_t offset, float32_t *dst,
				    uint32_t block_size)
{
	arm_offset_f32(src, offset, dst, block_size);
}

/* Clip (CMSIS-DSP only) */
static inline void zdsp_clip_q7(const q7_t *src, q7_t *dst, q7_t low, q7_t high,
				 uint32_t num_samples)
{
	arm_clip_q7(src, dst, low, high, num_samples);
}
static inline void zdsp_clip_q15(const q15_t *src, q15_t *dst, q15_t low, q15_t high,
				  uint32_t num_samples)
{
	arm_clip_q15(src, dst, low, high, num_samples);
}
static inline void zdsp_clip_q31(const q31_t *src, q31_t *dst, q31_t low, q31_t high,
				  uint32_t num_samples)
{
	arm_clip_q31(src, dst, low, high, num_samples);
}
static inline void zdsp_clip_f32(const float32_t *src, float32_t *dst, float32_t low,
				  float32_t high, uint32_t num_samples)
{
	arm_clip_f32(src, dst, low, high, num_samples);
}

/* Bitwise (CMSIS-DSP only) */
static inline void zdsp_and_u8(const uint8_t *src_a, const uint8_t *src_b, uint8_t *dst,
			       uint32_t block_size)
{
	arm_and_u8(src_a, src_b, dst, block_size);
}
static inline void zdsp_and_u16(const uint16_t *src_a, const uint16_t *src_b, uint16_t *dst,
				 uint32_t block_size)
{
	arm_and_u16(src_a, src_b, dst, block_size);
}
static inline void zdsp_and_u32(const uint32_t *src_a, const uint32_t *src_b, uint32_t *dst,
				 uint32_t block_size)
{
	arm_and_u32(src_a, src_b, dst, block_size);
}

static inline void zdsp_or_u8(const uint8_t *src_a, const uint8_t *src_b, uint8_t *dst,
			       uint32_t block_size)
{
	arm_or_u8(src_a, src_b, dst, block_size);
}
static inline void zdsp_or_u16(const uint16_t *src_a, const uint16_t *src_b, uint16_t *dst,
			       uint32_t block_size)
{
	arm_or_u16(src_a, src_b, dst, block_size);
}
static inline void zdsp_or_u32(const uint32_t *src_a, const uint32_t *src_b, uint32_t *dst,
			       uint32_t block_size)
{
	arm_or_u32(src_a, src_b, dst, block_size);
}

static inline void zdsp_xor_u8(const uint8_t *src_a, const uint8_t *src_b, uint8_t *dst,
			       uint32_t block_size)
{
	arm_xor_u8(src_a, src_b, dst, block_size);
}
static inline void zdsp_xor_u16(const uint16_t *src_a, const uint16_t *src_b, uint16_t *dst,
				 uint32_t block_size)
{
	arm_xor_u16(src_a, src_b, dst, block_size);
}
static inline void zdsp_xor_u32(const uint32_t *src_a, const uint32_t *src_b, uint32_t *dst,
				 uint32_t block_size)
{
	arm_xor_u32(src_a, src_b, dst, block_size);
}

static inline void zdsp_not_u8(const uint8_t *src, uint8_t *dst, uint32_t block_size)
{
	arm_not_u8(src, dst, block_size);
}
static inline void zdsp_not_u16(const uint16_t *src, uint16_t *dst, uint32_t block_size)
{
	arm_not_u16(src, dst, block_size);
}
static inline void zdsp_not_u32(const uint32_t *src, uint32_t *dst, uint32_t block_size)
{
	arm_not_u32(src, dst, block_size);
}

/*
 * Transform (FFT) functions.
 *
 * The PowerQuad has no transform engine the zdsp API can use directly, so these
 * wrap CMSIS-DSP, which this backend already depends on.
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

#ifdef CONFIG_FP16
#include "../../cmsis/public/zdsp_backend_f16.h"
#endif /* CONFIG_FP16 */

#endif /* SUBSYS_DSP_POWERQUAD_PUBLIC_ZDSP_BACKEND_H_ */
