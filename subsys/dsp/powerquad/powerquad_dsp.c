/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#include <arm_math.h>
#include <fsl_powerquad.h>

#ifdef CONFIG_DSP_POWERQUAD_ADDR_REMAP
#include <soc.h>
#else
static inline const void *soc_powerquad_remap_addr(const void *addr)
{
	return addr;
}
#endif

/* Get PowerQuad base address from devicetree */
#define PQ_BASE ((POWERQUAD_Type *)DT_REG_ADDR(DT_NODELABEL(powerquad)))

/* Mutex to protect PowerQuad hardware from concurrent access */
static K_MUTEX_DEFINE(pq_mutex);

/* PowerQuad tmp memory base. */
#define PQ_TMP_BASE ((uint32_t *)CONFIG_DSP_POWERQUAD_TMP_BASE)

/*
 * PowerQuad config patterns (ref: NXP SDK fsl_powerquad_cmsis.c).
 *
 * Register layout: (prescale << 8) | (external_format << 4) | machine_format
 * For non-FFT operations, machine_format must be kPQ_Float.
 *
 * PQ_F32_CONFIG:   float <==> float, no prescale
 * PQ_FIX32_CONFIG: Q31 <==> float,   no prescale  (add/sub/negate)
 * PQ_FIX16_CONFIG: Q15 <==> float,   no prescale  (add/sub/negate)
 * PQ_Q31_CONFIG:  Q31 <==> float,   output prescale=-31 (product/dot)
 * PQ_Q15_CONFIG:  Q15 <==> float,   output prescale=-15 (product/dot)
 */
static const pq_config_t pq_f32_config = {
	.inputAFormat = kPQ_Float,
	.inputAPrescale = 0,
	.inputBFormat = kPQ_Float,
	.inputBPrescale = 0,
	.outputFormat = kPQ_Float,
	.outputPrescale = 0,
	.tmpFormat = kPQ_Float,
	.tmpPrescale = 0,
	.machineFormat = kPQ_Float,
	.tmpBase = PQ_TMP_BASE,
};

/* For add/sub/negate — no output prescale */
static const pq_config_t pq_fix32_config = {
	.inputAFormat = kPQ_32Bit,
	.inputAPrescale = 0,
	.inputBFormat = kPQ_32Bit,
	.inputBPrescale = 0,
	.outputFormat = kPQ_32Bit,
	.outputPrescale = 0,
	.tmpFormat = kPQ_Float,
	.tmpPrescale = 0,
	.machineFormat = kPQ_Float,
	.tmpBase = PQ_TMP_BASE,
};

/* For add/sub/negate — no output prescale */
static const pq_config_t pq_fix16_config = {
	.inputAFormat = kPQ_16Bit,
	.inputAPrescale = 0,
	.inputBFormat = kPQ_16Bit,
	.inputBPrescale = 0,
	.outputFormat = kPQ_16Bit,
	.outputPrescale = 0,
	.tmpFormat = kPQ_Float,
	.tmpPrescale = 0,
	.machineFormat = kPQ_Float,
	.tmpBase = PQ_TMP_BASE,
};

/* For product/dot — output prescale=-31 */
static const pq_config_t pq_q31_config = {
	.inputAFormat = kPQ_32Bit,
	.inputAPrescale = 0,
	.inputBFormat = kPQ_32Bit,
	.inputBPrescale = 0,
	.outputFormat = kPQ_32Bit,
	.outputPrescale = -31,
	.tmpFormat = kPQ_Float,
	.tmpPrescale = 0,
	.machineFormat = kPQ_Float,
	.tmpBase = PQ_TMP_BASE,
};

/* For product/dot — output prescale=-15 */
static const pq_config_t pq_q15_config = {
	.inputAFormat = kPQ_16Bit,
	.inputAPrescale = 0,
	.inputBFormat = kPQ_16Bit,
	.inputBPrescale = 0,
	.outputFormat = kPQ_16Bit,
	.outputPrescale = -15,
	.tmpFormat = kPQ_Float,
	.tmpPrescale = 0,
	.machineFormat = kPQ_Float,
	.tmpBase = PQ_TMP_BASE,
};

/* Pointer typedefs to avoid 'type *' in macros (checkpatch false positive) */
typedef const float32_t *cp_f32_t;
typedef float32_t       *p_f32_t;
typedef const q31_t     *cp_q31_t;
typedef q31_t           *p_q31_t;
typedef const q15_t     *cp_q15_t;
typedef q15_t           *p_q15_t;

/*
 * Three-tier chunking for PowerQuad matrix engine:
 *
 * Tier 1: Full 16x16 chunks (256 elements) for maximum throughput
 * Tier 2: Remaining full rows of 16 (N x 16)
 * Tier 3: Tail < 16 elements via CMSIS-DSP
 */
#define PQ_MTX_COLS 16
#define PQ_MTX_ROWS 16
#define PQ_MTX_MAX  (PQ_MTX_ROWS * PQ_MTX_COLS)
#define PQ_DOT_MAX  256

/* PowerQuad matrix operation signature */
typedef void (*pq_binop_pq_fn)(POWERQUAD_Type *base, uint32_t length,
			       void *pData1, void *pData2, void *pResult);

/* CMSIS-DSP fallback signatures per type */
typedef void (*pq_binop_arm_fn_f32)(const float32_t *, const float32_t *,
				    float32_t *, uint32_t);
typedef void (*pq_binop_arm_fn_q31)(const q31_t *, const q31_t *,
				    q31_t *, uint32_t);
typedef void (*pq_binop_arm_fn_q15)(const q15_t *, const q15_t *,
				    q15_t *, uint32_t);

/*
 * Shared chunking engine for two-operand element-wise operations.
 *
 * PowerQuad's matrix engine supports 16x16 matrix size at most, so data is
 * processed in three tiers:
 * Tier 1: full 16x16 chunks (256 elements) for maximum throughput.
 * Tier 2: remaining full rows of 16 elements (N x 16).
 * Tier 3: tail < 16 elements via CMSIS-DSP software fallback.
 */
static void pq_binop_f32_t(pq_binop_pq_fn pq_func, const pq_config_t *cfg,
			   pq_binop_arm_fn_f32 arm_func,
			   cp_f32_t src_a, cp_f32_t src_b,
			   p_f32_t dst, uint32_t block_size)
{
	uint32_t remaining = block_size;
	uint32_t full_rows, count, len_full, len_partial;
	cp_f32_t hw_a = soc_powerquad_remap_addr(src_a);
	cp_f32_t hw_b = soc_powerquad_remap_addr(src_b);

	if (remaining >= PQ_MTX_COLS) {
		len_full = POWERQUAD_MAKE_MATRIX_LEN(PQ_MTX_ROWS, PQ_MTX_COLS,
						     PQ_MTX_COLS);
		k_mutex_lock(&pq_mutex, K_FOREVER);
		PQ_SetConfig(PQ_BASE, cfg);

		while (remaining >= PQ_MTX_MAX) {
			pq_func(PQ_BASE, len_full, (void *)hw_a,
				(void *)hw_b, (void *)dst);
			hw_a += PQ_MTX_MAX;
			hw_b += PQ_MTX_MAX;
			dst += PQ_MTX_MAX;
			remaining -= PQ_MTX_MAX;
			PQ_WaitDone(PQ_BASE);
		}

		full_rows = remaining / PQ_MTX_COLS;
		if (full_rows > 0) {
			count = full_rows * PQ_MTX_COLS;
			len_partial = POWERQUAD_MAKE_MATRIX_LEN(
				full_rows, PQ_MTX_COLS, PQ_MTX_COLS);
			pq_func(PQ_BASE, len_partial, (void *)hw_a,
				(void *)hw_b, (void *)dst);
			dst += count;
			remaining -= count;
			PQ_WaitDone(PQ_BASE);
		}
		k_mutex_unlock(&pq_mutex);
	}

	if (remaining > 0) {
		arm_func(src_a + (block_size - remaining),
			 src_b + (block_size - remaining), dst, remaining);
	}
}

static void pq_binop_q31_t(pq_binop_pq_fn pq_func, const pq_config_t *cfg,
			   pq_binop_arm_fn_q31 arm_func,
			   cp_q31_t src_a, cp_q31_t src_b,
			   p_q31_t dst, uint32_t block_size)
{
	uint32_t remaining = block_size;
	uint32_t full_rows, count, len_full, len_partial;
	cp_q31_t hw_a = soc_powerquad_remap_addr(src_a);
	cp_q31_t hw_b = soc_powerquad_remap_addr(src_b);

	if (remaining >= PQ_MTX_COLS) {
		len_full = POWERQUAD_MAKE_MATRIX_LEN(PQ_MTX_ROWS, PQ_MTX_COLS,
						     PQ_MTX_COLS);
		k_mutex_lock(&pq_mutex, K_FOREVER);
		PQ_SetConfig(PQ_BASE, cfg);

		while (remaining >= PQ_MTX_MAX) {
			pq_func(PQ_BASE, len_full, (void *)hw_a,
				(void *)hw_b, (void *)dst);
			hw_a += PQ_MTX_MAX;
			hw_b += PQ_MTX_MAX;
			dst += PQ_MTX_MAX;
			remaining -= PQ_MTX_MAX;
			PQ_WaitDone(PQ_BASE);
		}

		full_rows = remaining / PQ_MTX_COLS;
		if (full_rows > 0) {
			count = full_rows * PQ_MTX_COLS;
			len_partial = POWERQUAD_MAKE_MATRIX_LEN(
				full_rows, PQ_MTX_COLS, PQ_MTX_COLS);
			pq_func(PQ_BASE, len_partial, (void *)hw_a,
				(void *)hw_b, (void *)dst);
			dst += count;
			remaining -= count;
			PQ_WaitDone(PQ_BASE);
		}
		k_mutex_unlock(&pq_mutex);
	}

	if (remaining > 0) {
		arm_func(src_a + (block_size - remaining),
			 src_b + (block_size - remaining), dst, remaining);
	}
}

static void pq_binop_q15_t(pq_binop_pq_fn pq_func, const pq_config_t *cfg,
			   pq_binop_arm_fn_q15 arm_func,
			   cp_q15_t src_a, cp_q15_t src_b,
			   p_q15_t dst, uint32_t block_size)
{
	uint32_t remaining = block_size;
	uint32_t full_rows, count, len_full, len_partial;
	cp_q15_t hw_a = soc_powerquad_remap_addr(src_a);
	cp_q15_t hw_b = soc_powerquad_remap_addr(src_b);

	if (remaining >= PQ_MTX_COLS) {
		len_full = POWERQUAD_MAKE_MATRIX_LEN(PQ_MTX_ROWS, PQ_MTX_COLS,
						     PQ_MTX_COLS);
		k_mutex_lock(&pq_mutex, K_FOREVER);
		PQ_SetConfig(PQ_BASE, cfg);

		while (remaining >= PQ_MTX_MAX) {
			pq_func(PQ_BASE, len_full, (void *)hw_a,
				(void *)hw_b, (void *)dst);
			hw_a += PQ_MTX_MAX;
			hw_b += PQ_MTX_MAX;
			dst += PQ_MTX_MAX;
			remaining -= PQ_MTX_MAX;
			PQ_WaitDone(PQ_BASE);
		}

		full_rows = remaining / PQ_MTX_COLS;
		if (full_rows > 0) {
			count = full_rows * PQ_MTX_COLS;
			len_partial = POWERQUAD_MAKE_MATRIX_LEN(
				full_rows, PQ_MTX_COLS, PQ_MTX_COLS);
			pq_func(PQ_BASE, len_partial, (void *)hw_a,
				(void *)hw_b, (void *)dst);
			dst += count;
			remaining -= count;
			PQ_WaitDone(PQ_BASE);
		}
		k_mutex_unlock(&pq_mutex);
	}

	if (remaining > 0) {
		arm_func(src_a + (block_size - remaining),
			 src_b + (block_size - remaining), dst, remaining);
	}
}

/*
 * PQ_DEFINE_BINOP - emit a named public function that delegates to
 * the typed static helper. No logic lives in the macro.
 */
#define PQ_DEFINE_BINOP(name, pq_func, pq_cfg, arm_func, type)		\
	void name(cp_##type src_a, cp_##type src_b,			\
		  p_##type dst, uint32_t block_size)			\
	{								\
		pq_binop_##type(pq_func, &pq_cfg, arm_func,		\
				src_a, src_b, dst, block_size);		\
	}

/* Multiply */
PQ_DEFINE_BINOP(pq_mult_f32, PQ_MatrixProduct, pq_f32_config, arm_mult_f32, f32_t)
PQ_DEFINE_BINOP(pq_mult_q31, PQ_MatrixProduct, pq_q31_config, arm_mult_q31, q31_t)
PQ_DEFINE_BINOP(pq_mult_q15, PQ_MatrixProduct, pq_q15_config, arm_mult_q15, q15_t)

/* Add */
PQ_DEFINE_BINOP(pq_add_f32, PQ_MatrixAddition, pq_f32_config, arm_add_f32, f32_t)
PQ_DEFINE_BINOP(pq_add_q31, PQ_MatrixAddition, pq_fix32_config, arm_add_q31, q31_t)
PQ_DEFINE_BINOP(pq_add_q15, PQ_MatrixAddition, pq_fix16_config, arm_add_q15, q15_t)

/* Subtract */
PQ_DEFINE_BINOP(pq_sub_f32, PQ_MatrixSubtraction, pq_f32_config, arm_sub_f32, f32_t)
PQ_DEFINE_BINOP(pq_sub_q31, PQ_MatrixSubtraction, pq_fix32_config, arm_sub_q31, q31_t)
PQ_DEFINE_BINOP(pq_sub_q15, PQ_MatrixSubtraction, pq_fix16_config, arm_sub_q15, q15_t)

/* Scale (f32) */
void pq_scale_f32(const float32_t *src, float32_t scale, float32_t *dst,
		  uint32_t block_size)
{
	uint32_t remaining = block_size;
	const float32_t *hw_src = soc_powerquad_remap_addr(src);

	if (remaining >= PQ_MTX_COLS) {
		uint32_t len_full =
			POWERQUAD_MAKE_MATRIX_LEN(PQ_MTX_ROWS, PQ_MTX_COLS, 0);

		k_mutex_lock(&pq_mutex, K_FOREVER);
		PQ_SetConfig(PQ_BASE, &pq_f32_config);

		while (remaining >= PQ_MTX_MAX) {
			PQ_MatrixScale(PQ_BASE, len_full, scale,
				       (void *)hw_src, (void *)dst);
			hw_src += PQ_MTX_MAX;
			dst += PQ_MTX_MAX;
			remaining -= PQ_MTX_MAX;
			PQ_WaitDone(PQ_BASE);
		}

		uint32_t full_rows = remaining / PQ_MTX_COLS;

		if (full_rows > 0) {
			uint32_t len_partial = POWERQUAD_MAKE_MATRIX_LEN(
				full_rows, PQ_MTX_COLS, 0);
			uint32_t count = full_rows * PQ_MTX_COLS;

			PQ_MatrixScale(PQ_BASE, len_partial, scale,
				       (void *)hw_src, (void *)dst);
			dst += count;
			remaining -= count;
			PQ_WaitDone(PQ_BASE);
		}
		k_mutex_unlock(&pq_mutex);
	}

	if (remaining > 0) {
		arm_scale_f32(src + (block_size - remaining), scale, dst, remaining);
	}
}

/* Scale (q31) */
void pq_scale_q31(const q31_t *src, q31_t scale_fract, int8_t shift, q31_t *dst,
		  uint32_t block_size)
{
	uint32_t remaining = block_size;
	const q31_t *hw_src = soc_powerquad_remap_addr(src);

	if (remaining >= PQ_MTX_COLS) {
		pq_config_t config = {
			.inputAFormat = kPQ_32Bit,
			.inputAPrescale = 0,
			.inputBFormat = kPQ_32Bit,
			.inputBPrescale = 0,
			.outputFormat = kPQ_32Bit,
			.outputPrescale = (int8_t)shift,
			.tmpFormat = kPQ_Float,
			.tmpPrescale = 0,
			.machineFormat = kPQ_Float,
			.tmpBase = PQ_TMP_BASE,
		};
		float scale_float = PQ_Q31_2_FLOAT(scale_fract);
		uint32_t len_full =
			POWERQUAD_MAKE_MATRIX_LEN(PQ_MTX_ROWS, PQ_MTX_COLS, 0);

		k_mutex_lock(&pq_mutex, K_FOREVER);
		PQ_SetConfig(PQ_BASE, &config);

		while (remaining >= PQ_MTX_MAX) {
			PQ_MatrixScale(PQ_BASE, len_full, scale_float,
				       (void *)hw_src, (void *)dst);
			hw_src += PQ_MTX_MAX;
			dst += PQ_MTX_MAX;
			remaining -= PQ_MTX_MAX;
			PQ_WaitDone(PQ_BASE);
		}

		uint32_t full_rows = remaining / PQ_MTX_COLS;

		if (full_rows > 0) {
			uint32_t len_partial = POWERQUAD_MAKE_MATRIX_LEN(
				full_rows, PQ_MTX_COLS, 0);
			uint32_t count = full_rows * PQ_MTX_COLS;

			PQ_MatrixScale(PQ_BASE, len_partial, scale_float,
				       (void *)hw_src, (void *)dst);
			dst += count;
			remaining -= count;
			PQ_WaitDone(PQ_BASE);
		}
		k_mutex_unlock(&pq_mutex);
	}

	if (remaining > 0) {
		arm_scale_q31(src + (block_size - remaining), scale_fract, shift,
			     dst, remaining);
	}
}

/* Scale (q15) */
void pq_scale_q15(const q15_t *src, q15_t scale_fract, int8_t shift, q15_t *dst,
		  uint32_t block_size)
{
	uint32_t remaining = block_size;
	const q15_t *hw_src = soc_powerquad_remap_addr(src);

	if (remaining >= PQ_MTX_COLS) {
		pq_config_t config = {
			.inputAFormat = kPQ_16Bit,
			.inputAPrescale = 0,
			.inputBFormat = kPQ_16Bit,
			.inputBPrescale = 0,
			.outputFormat = kPQ_16Bit,
			.outputPrescale = (int8_t)shift,
			.tmpFormat = kPQ_Float,
			.tmpPrescale = 0,
			.machineFormat = kPQ_Float,
			.tmpBase = PQ_TMP_BASE,
		};
		float scale_float = PQ_Q15_2_FLOAT(scale_fract);
		uint32_t len_full =
			POWERQUAD_MAKE_MATRIX_LEN(PQ_MTX_ROWS, PQ_MTX_COLS, 0);

		k_mutex_lock(&pq_mutex, K_FOREVER);
		PQ_SetConfig(PQ_BASE, &config);

		while (remaining >= PQ_MTX_MAX) {
			PQ_MatrixScale(PQ_BASE, len_full, scale_float,
				       (void *)hw_src, (void *)dst);
			hw_src += PQ_MTX_MAX;
			dst += PQ_MTX_MAX;
			remaining -= PQ_MTX_MAX;
			PQ_WaitDone(PQ_BASE);
		}

		uint32_t full_rows = remaining / PQ_MTX_COLS;

		if (full_rows > 0) {
			uint32_t len_partial = POWERQUAD_MAKE_MATRIX_LEN(
				full_rows, PQ_MTX_COLS, 0);
			uint32_t count = full_rows * PQ_MTX_COLS;

			PQ_MatrixScale(PQ_BASE, len_partial, scale_float,
				       (void *)hw_src, (void *)dst);
			dst += count;
			remaining -= count;
			PQ_WaitDone(PQ_BASE);
		}
		k_mutex_unlock(&pq_mutex);
	}

	if (remaining > 0) {
		arm_scale_q15(src + (block_size - remaining), scale_fract, shift,
			     dst, remaining);
	}
}

/* Negate (f32: PQ scale by -1.0) */
void pq_negate_f32(const float32_t *src, float32_t *dst, uint32_t block_size)
{
	uint32_t remaining = block_size;
	const float32_t *hw_src = soc_powerquad_remap_addr(src);

	if (remaining >= PQ_MTX_COLS) {
		uint32_t len_full =
			POWERQUAD_MAKE_MATRIX_LEN(PQ_MTX_ROWS, PQ_MTX_COLS, 0);

		k_mutex_lock(&pq_mutex, K_FOREVER);
		PQ_SetConfig(PQ_BASE, &pq_f32_config);

		while (remaining >= PQ_MTX_MAX) {
			PQ_MatrixScale(PQ_BASE, len_full, -1.0f,
				       (void *)hw_src, (void *)dst);
			hw_src += PQ_MTX_MAX;
			dst += PQ_MTX_MAX;
			remaining -= PQ_MTX_MAX;
			PQ_WaitDone(PQ_BASE);
		}

		uint32_t full_rows = remaining / PQ_MTX_COLS;

		if (full_rows > 0) {
			uint32_t len_partial = POWERQUAD_MAKE_MATRIX_LEN(
				full_rows, PQ_MTX_COLS, 0);
			uint32_t count = full_rows * PQ_MTX_COLS;

			PQ_MatrixScale(PQ_BASE, len_partial, -1.0f,
				       (void *)hw_src, (void *)dst);
			dst += count;
			remaining -= count;
			PQ_WaitDone(PQ_BASE);
		}
		k_mutex_unlock(&pq_mutex);
	}

	if (remaining > 0) {
		arm_negate_f32(src + (block_size - remaining), dst, remaining);
	}
}

/* Negate (q31: PQ scale by -1.0 with FIX32 config) */
void pq_negate_q31(const q31_t *src, q31_t *dst, uint32_t block_size)
{
	uint32_t remaining = block_size;
	const q31_t *hw_src = soc_powerquad_remap_addr(src);

	if (remaining >= PQ_MTX_COLS) {
		uint32_t len_full =
			POWERQUAD_MAKE_MATRIX_LEN(PQ_MTX_ROWS, PQ_MTX_COLS, 0);

		k_mutex_lock(&pq_mutex, K_FOREVER);
		PQ_SetConfig(PQ_BASE, &pq_fix32_config);

		while (remaining >= PQ_MTX_MAX) {
			PQ_MatrixScale(PQ_BASE, len_full, -1.0f,
				       (void *)hw_src, (void *)dst);
			hw_src += PQ_MTX_MAX;
			dst += PQ_MTX_MAX;
			remaining -= PQ_MTX_MAX;
			PQ_WaitDone(PQ_BASE);
		}

		uint32_t full_rows = remaining / PQ_MTX_COLS;

		if (full_rows > 0) {
			uint32_t len_partial = POWERQUAD_MAKE_MATRIX_LEN(
				full_rows, PQ_MTX_COLS, 0);
			uint32_t count = full_rows * PQ_MTX_COLS;

			PQ_MatrixScale(PQ_BASE, len_partial, -1.0f,
				       (void *)hw_src, (void *)dst);
			dst += count;
			remaining -= count;
			PQ_WaitDone(PQ_BASE);
		}
		k_mutex_unlock(&pq_mutex);
	}

	if (remaining > 0) {
		arm_negate_q31(src + (block_size - remaining), dst, remaining);
	}
}

/* Negate (q15: PQ scale by -1.0 with FIX16 config) */
void pq_negate_q15(const q15_t *src, q15_t *dst, uint32_t block_size)
{
	uint32_t remaining = block_size;
	const q15_t *hw_src = soc_powerquad_remap_addr(src);

	if (remaining >= PQ_MTX_COLS) {
		uint32_t len_full =
			POWERQUAD_MAKE_MATRIX_LEN(PQ_MTX_ROWS, PQ_MTX_COLS, 0);

		k_mutex_lock(&pq_mutex, K_FOREVER);
		PQ_SetConfig(PQ_BASE, &pq_fix16_config);

		while (remaining >= PQ_MTX_MAX) {
			PQ_MatrixScale(PQ_BASE, len_full, -1.0f,
				       (void *)hw_src, (void *)dst);
			hw_src += PQ_MTX_MAX;
			dst += PQ_MTX_MAX;
			remaining -= PQ_MTX_MAX;
			PQ_WaitDone(PQ_BASE);
		}

		uint32_t full_rows = remaining / PQ_MTX_COLS;

		if (full_rows > 0) {
			uint32_t len_partial = POWERQUAD_MAKE_MATRIX_LEN(
				full_rows, PQ_MTX_COLS, 0);
			uint32_t count = full_rows * PQ_MTX_COLS;

			PQ_MatrixScale(PQ_BASE, len_partial, -1.0f,
				       (void *)hw_src, (void *)dst);
			dst += count;
			remaining -= count;
			PQ_WaitDone(PQ_BASE);
		}
		k_mutex_unlock(&pq_mutex);
	}

	if (remaining > 0) {
		arm_negate_q15(src + (block_size - remaining), dst, remaining);
	}
}

/* Dot product (f32: PQ chunked x256) */
void pq_dot_prod_f32(const float32_t *src_a, const float32_t *src_b,
		     uint32_t block_size, float32_t *dst)
{
	float32_t sum = 0.0f;
	uint32_t remaining = block_size;
	const float32_t *hw_a = soc_powerquad_remap_addr(src_a);
	const float32_t *hw_b = soc_powerquad_remap_addr(src_b);

	if (remaining >= PQ_DOT_MAX) {
		float32_t partial;

		k_mutex_lock(&pq_mutex, K_FOREVER);
		PQ_SetConfig(PQ_BASE, &pq_f32_config);

		while (remaining >= PQ_DOT_MAX) {
			PQ_VectorDotProduct(PQ_BASE, PQ_DOT_MAX, (void *)hw_a,
					    (void *)hw_b, (void *)&partial);
			hw_a += PQ_DOT_MAX;
			hw_b += PQ_DOT_MAX;
			remaining -= PQ_DOT_MAX;
			PQ_WaitDone(PQ_BASE);
			sum += partial;
		}
		k_mutex_unlock(&pq_mutex);
	}

	if (remaining > 0) {
		float32_t tail;

		arm_dot_prod_f32(src_a + (block_size - remaining),
				 src_b + (block_size - remaining), remaining, &tail);
		sum += tail;
	}

	*dst = sum;
}
