/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zephyr/dsp/transform_f16.h
 *
 * @brief Public APIs for DSP transform (FFT) functions for 16 bit floating point
 */

#ifndef ZEPHYR_INCLUDE_DSP_TRANSFORM_F16_H_
#define ZEPHYR_INCLUDE_DSP_TRANSFORM_F16_H_

#ifndef CONFIG_FP16
#error "Cannot use float16 DSP functionality without CONFIG_FP16 enabled"
#endif /* CONFIG_FP16 */

#include <stdint.h>

#include <zephyr/dsp/dsp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Instance structure for the f16 complex FFT.
 *
 * Opaque type; the concrete definition is provided by the selected DSP backend.
 */
struct zdsp_cfft_instance_f16;

/**
 * @brief Instance structure for the f16 real (fast) FFT.
 *
 * Opaque type; the concrete definition is provided by the selected DSP backend.
 */
struct zdsp_rfft_fast_instance_f16;

/**
 * @ingroup math_dsp_transform_cfft
 * @brief Initialize an f16 complex FFT instance.
 *
 * @param[out] inst     points to the instance to initialize
 * @param[in]  fft_len  length of the FFT (number of complex samples, a power of two)
 *
 * @retval ZDSP_TRANSFORM_STATUS_OK Success.
 * @retval ZDSP_TRANSFORM_STATUS_ERROR Unsupported transform length.
 */
DSP_FUNC_SCOPE zdsp_transform_status zdsp_cfft_init_f16(struct zdsp_cfft_instance_f16 *inst,
							uint16_t fft_len);

/**
 * @ingroup math_dsp_transform_cfft
 * @brief f16 complex FFT.
 *
 * @param[in]     inst              points to an initialized instance
 * @param[in,out] p                 points to the in-place interleaved complex buffer
 * @param[in]     ifft_flag         0 for forward transform, 1 for inverse transform
 * @param[in]     bit_reverse_flag  1 to enable output bit reversal, 0 to disable
 */
DSP_FUNC_SCOPE void zdsp_cfft_f16(const struct zdsp_cfft_instance_f16 *inst, float16_t *p,
				  uint8_t ifft_flag, uint8_t bit_reverse_flag);

/**
 * @ingroup math_dsp_transform_rfft
 * @brief Initialize an f16 real (fast) FFT instance.
 *
 * @param[out] inst     points to the instance to initialize
 * @param[in]  fft_len  length of the real FFT (a power of two)
 *
 * @retval ZDSP_TRANSFORM_STATUS_OK Success.
 * @retval ZDSP_TRANSFORM_STATUS_ERROR Unsupported transform length.
 */
DSP_FUNC_SCOPE zdsp_transform_status
zdsp_rfft_fast_init_f16(struct zdsp_rfft_fast_instance_f16 *inst, uint16_t fft_len);

/**
 * @ingroup math_dsp_transform_rfft
 * @brief f16 real (fast) FFT.
 *
 * @param[in]     inst       points to an initialized instance
 * @param[in,out] p          points to the input buffer
 * @param[out]    out        points to the output buffer
 * @param[in]     ifft_flag  0 for forward transform, 1 for inverse transform
 */
DSP_FUNC_SCOPE void zdsp_rfft_fast_f16(const struct zdsp_rfft_fast_instance_f16 *inst, float16_t *p,
				       float16_t *out, uint8_t ifft_flag);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DSP_TRANSFORM_F16_H_ */
