/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for DSP transform (FFT) functions
 * @ingroup math_dsp_transform
 */

#ifndef ZEPHYR_INCLUDE_DSP_TRANSFORM_H_
#define ZEPHYR_INCLUDE_DSP_TRANSFORM_H_

#include <stdint.h>

#include <zephyr/dsp/dsp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup math_dsp
 * @defgroup math_dsp_transform Transform Functions
 *
 * Fast Fourier Transform (FFT) functions for complex and real signals.
 *
 * Each transform operates on an @em instance which must be initialized once (for a given
 * transform length) with the matching `zdsp_*_init_*` function before it is used. The
 * instance holds the twiddle factor and bit-reversal tables required by the transform and
 * may be reused for any number of subsequent transforms of the same length.
 *
 * Transform lengths must be a power of two. Complex transforms operate in place on an
 * interleaved `{real, imag}` buffer of `2 * fft_len` samples. Real transforms read
 * `fft_len` real samples and produce a packed complex spectrum.
 *
 * The concrete layout of the instance structures is provided by the selected DSP backend;
 * applications should treat them as opaque and only allocate storage for them (e.g. on the
 * stack) and pass pointers to the API.
 *
 * The half-precision (f16) transforms are declared in
 * @ref zephyr/dsp/transform_f16.h, which this header includes when
 * @kconfig{CONFIG_FP16} is enabled.
 * @{
 */

/**
 * @brief Status code returned by the transform initialization functions.
 */
typedef enum {
	/** @brief Initialization succeeded. */
	ZDSP_TRANSFORM_STATUS_OK = 0,
	/** @brief Initialization failed (e.g. unsupported transform length). */
	ZDSP_TRANSFORM_STATUS_ERROR = -1,
} zdsp_transform_status;

/**
 * @brief Instance structure for the Q15 complex FFT.
 *
 * Opaque type; the concrete definition is provided by the selected DSP backend.
 */
struct zdsp_cfft_instance_q15;

/**
 * @brief Instance structure for the Q31 complex FFT.
 *
 * Opaque type; the concrete definition is provided by the selected DSP backend.
 */
struct zdsp_cfft_instance_q31;

/**
 * @brief Instance structure for the f32 complex FFT.
 *
 * Opaque type; the concrete definition is provided by the selected DSP backend.
 */
struct zdsp_cfft_instance_f32;

/**
 * @brief Instance structure for the Q15 real FFT.
 *
 * Opaque type; the concrete definition is provided by the selected DSP backend.
 */
struct zdsp_rfft_instance_q15;

/**
 * @brief Instance structure for the Q31 real FFT.
 *
 * Opaque type; the concrete definition is provided by the selected DSP backend.
 */
struct zdsp_rfft_instance_q31;

/**
 * @brief Instance structure for the f32 real (fast) FFT.
 *
 * Opaque type; the concrete definition is provided by the selected DSP backend.
 */
struct zdsp_rfft_fast_instance_f32;

/**
 * @ingroup math_dsp_transform
 * @defgroup math_dsp_transform_cfft Complex FFT
 *
 * In-place complex Fast Fourier Transform. The input/output buffer holds @c fft_len
 * complex samples stored as interleaved real and imaginary parts, i.e. @c 2*fft_len
 * elements.
 *
 * The fixed-point transforms are downscaled by one bit per stage to avoid saturation, so
 * both the forward and the inverse transform carry a @c 1/fft_len scaling. The
 * floating-point forward transform is unscaled and only the inverse transform is
 * normalized by @c 1/fft_len.
 * @{
 */

/**
 * @brief Initialize a Q15 complex FFT instance.
 *
 * @param[out] inst     points to the instance to initialize
 * @param[in]  fft_len  length of the FFT (number of complex samples, a power of two)
 *
 * @retval ZDSP_TRANSFORM_STATUS_OK Success.
 * @retval ZDSP_TRANSFORM_STATUS_ERROR Unsupported transform length.
 */
DSP_FUNC_SCOPE zdsp_transform_status zdsp_cfft_init_q15(struct zdsp_cfft_instance_q15 *inst,
							uint16_t fft_len);

/**
 * @brief Q15 complex FFT.
 *
 * @param[in]     inst              points to an initialized instance
 * @param[in,out] p                 points to the in-place interleaved complex buffer
 * @param[in]     ifft_flag         0 for forward transform, 1 for inverse transform
 * @param[in]     bit_reverse_flag  1 to enable output bit reversal, 0 to disable
 */
DSP_FUNC_SCOPE void zdsp_cfft_q15(const struct zdsp_cfft_instance_q15 *inst, q15_t *p,
				  uint8_t ifft_flag, uint8_t bit_reverse_flag);

/**
 * @brief Initialize a Q31 complex FFT instance.
 *
 * @param[out] inst     points to the instance to initialize
 * @param[in]  fft_len  length of the FFT (number of complex samples, a power of two)
 *
 * @retval ZDSP_TRANSFORM_STATUS_OK Success.
 * @retval ZDSP_TRANSFORM_STATUS_ERROR Unsupported transform length.
 */
DSP_FUNC_SCOPE zdsp_transform_status zdsp_cfft_init_q31(struct zdsp_cfft_instance_q31 *inst,
							uint16_t fft_len);

/**
 * @brief Q31 complex FFT.
 *
 * @param[in]     inst              points to an initialized instance
 * @param[in,out] p                 points to the in-place interleaved complex buffer
 * @param[in]     ifft_flag         0 for forward transform, 1 for inverse transform
 * @param[in]     bit_reverse_flag  1 to enable output bit reversal, 0 to disable
 */
DSP_FUNC_SCOPE void zdsp_cfft_q31(const struct zdsp_cfft_instance_q31 *inst, q31_t *p,
				  uint8_t ifft_flag, uint8_t bit_reverse_flag);

/**
 * @brief Initialize an f32 complex FFT instance.
 *
 * @param[out] inst     points to the instance to initialize
 * @param[in]  fft_len  length of the FFT (number of complex samples, a power of two)
 *
 * @retval ZDSP_TRANSFORM_STATUS_OK Success.
 * @retval ZDSP_TRANSFORM_STATUS_ERROR Unsupported transform length.
 */
DSP_FUNC_SCOPE zdsp_transform_status zdsp_cfft_init_f32(struct zdsp_cfft_instance_f32 *inst,
							uint16_t fft_len);

/**
 * @brief f32 complex FFT.
 *
 * @param[in]     inst              points to an initialized instance
 * @param[in,out] p                 points to the in-place interleaved complex buffer
 * @param[in]     ifft_flag         0 for forward transform, 1 for inverse transform
 * @param[in]     bit_reverse_flag  1 to enable output bit reversal, 0 to disable
 */
DSP_FUNC_SCOPE void zdsp_cfft_f32(const struct zdsp_cfft_instance_f32 *inst, float32_t *p,
				  uint8_t ifft_flag, uint8_t bit_reverse_flag);

/**
 * @}
 */

/**
 * @ingroup math_dsp_transform
 * @defgroup math_dsp_transform_rfft Real FFT
 *
 * Real Fast Fourier Transform. The forward transform takes @c fft_len real samples and
 * produces a complex spectrum; the inverse transform reverses the operation.
 *
 * The fixed-point and floating-point variants do not use the same output layout. The
 * fixed-point forward transform writes the full conjugate-symmetric spectrum, i.e.
 * @c fft_len complex bins (@c 2*fft_len elements), and its inverse reads the first
 * @c fft_len+2 elements. The floating-point (fast) transform packs the spectrum into
 * @c fft_len elements, with the real DC term in element 0 and the real Nyquist term in
 * element 1.
 * @{
 */

/**
 * @brief Initialize a Q15 real FFT instance.
 *
 * @param[out] inst              points to the instance to initialize
 * @param[in]  fft_len           length of the real FFT (a power of two)
 * @param[in]  ifft_flag         0 for forward transform, 1 for inverse transform
 * @param[in]  bit_reverse_flag  1 to enable bit reversal, 0 to disable
 *
 * @retval ZDSP_TRANSFORM_STATUS_OK Success.
 * @retval ZDSP_TRANSFORM_STATUS_ERROR Unsupported transform length or direction.
 */
DSP_FUNC_SCOPE zdsp_transform_status zdsp_rfft_init_q15(struct zdsp_rfft_instance_q15 *inst,
							uint32_t fft_len, uint32_t ifft_flag,
							uint32_t bit_reverse_flag);

/**
 * @brief Q15 real FFT.
 *
 * @param[in]  inst  points to an initialized instance
 * @param[in]  src   points to the input buffer
 * @param[out] dst   points to the output buffer
 */
DSP_FUNC_SCOPE void zdsp_rfft_q15(const struct zdsp_rfft_instance_q15 *inst, q15_t *src,
				  q15_t *dst);

/**
 * @brief Initialize a Q31 real FFT instance.
 *
 * @param[out] inst              points to the instance to initialize
 * @param[in]  fft_len           length of the real FFT (a power of two)
 * @param[in]  ifft_flag         0 for forward transform, 1 for inverse transform
 * @param[in]  bit_reverse_flag  1 to enable bit reversal, 0 to disable
 *
 * @retval ZDSP_TRANSFORM_STATUS_OK Success.
 * @retval ZDSP_TRANSFORM_STATUS_ERROR Unsupported transform length or direction.
 */
DSP_FUNC_SCOPE zdsp_transform_status zdsp_rfft_init_q31(struct zdsp_rfft_instance_q31 *inst,
							uint32_t fft_len, uint32_t ifft_flag,
							uint32_t bit_reverse_flag);

/**
 * @brief Q31 real FFT.
 *
 * @param[in]  inst  points to an initialized instance
 * @param[in]  src   points to the input buffer
 * @param[out] dst   points to the output buffer
 */
DSP_FUNC_SCOPE void zdsp_rfft_q31(const struct zdsp_rfft_instance_q31 *inst, q31_t *src,
				  q31_t *dst);

/**
 * @brief Initialize an f32 real (fast) FFT instance.
 *
 * @param[out] inst     points to the instance to initialize
 * @param[in]  fft_len  length of the real FFT (a power of two)
 *
 * @retval ZDSP_TRANSFORM_STATUS_OK Success.
 * @retval ZDSP_TRANSFORM_STATUS_ERROR Unsupported transform length.
 */
DSP_FUNC_SCOPE zdsp_transform_status
zdsp_rfft_fast_init_f32(struct zdsp_rfft_fast_instance_f32 *inst, uint16_t fft_len);

/**
 * @brief f32 real (fast) FFT.
 *
 * @param[in]     inst       points to an initialized instance
 * @param[in,out] p          points to the input buffer
 * @param[out]    out        points to the output buffer
 * @param[in]     ifft_flag  0 for forward transform, 1 for inverse transform
 */
DSP_FUNC_SCOPE void zdsp_rfft_fast_f32(const struct zdsp_rfft_fast_instance_f32 *inst, float32_t *p,
				       float32_t *out, uint8_t ifft_flag);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#ifdef CONFIG_FP16
#include <zephyr/dsp/transform_f16.h>
#endif /* CONFIG_FP16 */

#endif /* ZEPHYR_INCLUDE_DSP_TRANSFORM_H_ */
