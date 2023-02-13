/* Copyright (c) 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_DSP_CMSIS_PUBLIC_ZDSP_BACKEND_F16_H_
#define SUBSYS_DSP_CMSIS_PUBLIC_ZDSP_BACKEND_F16_H_

#ifdef __cplusplus
extern "C" {
#endif

/* This include MUST be done before arm_math.h so we can let the arch specific
 * logic set up the right #define values for arm_math.h
 */
#include <zephyr/kernel.h>

#include <arm_math_f16.h>

static inline void zdsp_mult_f16(const float16_t *src_a, const float16_t *src_b, float16_t *dst,
				 uint32_t block_size)
{
	arm_mult_f16(src_a, src_b, dst, block_size);
}

static inline void zdsp_add_f16(const float16_t *src_a, const float16_t *src_b, float16_t *dst,
				uint32_t block_size)
{
	arm_add_f16(src_a, src_b, dst, block_size);
}

static inline void zdsp_sub_f16(const float16_t *src_a, const float16_t *src_b, float16_t *dst,
				uint32_t block_size)
{
	arm_sub_f16(src_a, src_b, dst, block_size);
}

static inline void zdsp_scale_f16(const float16_t *src, float16_t scale, float16_t *dst,
				  uint32_t block_size)
{
	arm_scale_f16(src, scale, dst, block_size);
}

static inline void zdsp_abs_f16(const float16_t *src, float16_t *dst, uint32_t block_size)
{
	arm_abs_f16(src, dst, block_size);
}

static inline void zdsp_dot_prod_f16(const float16_t *src_a, const float16_t *src_b,
				     uint32_t block_size, float16_t *result)
{
	arm_dot_prod_f16(src_a, src_b, block_size, result);
}

static inline void zdsp_offset_f16(const float16_t *src, float16_t offset, float16_t *dst,
				   uint32_t block_size)
{
	arm_offset_f16(src, offset, dst, block_size);
}

static inline void zdsp_negate_f16(const float16_t *src, float16_t *dst, uint32_t block_size)
{
	arm_negate_f16(src, dst, block_size);
}

static inline void zdsp_clip_f16(const float16_t *src, float16_t *dst, float16_t low,
				 float16_t high, uint32_t num_samples)
{
	arm_clip_f16(src, dst, low, high, num_samples);
}

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_DSP_CMSIS_PUBLIC_ZDSP_BACKEND_F16_H_ */
