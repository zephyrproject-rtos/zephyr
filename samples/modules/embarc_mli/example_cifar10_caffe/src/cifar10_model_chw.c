/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cifar10_model.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mli_api.h"
#include "mli_types.h"
#include "mli_config.h"

#include "cifar10_constants.h"
#include "tests_aux.h"

#if (MODEL_BIT_DEPTH == MODEL_FX_8)
#define D_EL_TYPE (MLI_EL_FX_8)
#else
#define D_EL_TYPE (MLI_EL_FX_16)
#endif

/* -------------------------------------------------------------------------- */
/*                         Data related to the Module                         */
/* -------------------------------------------------------------------------- */

/* Intermediate data buffers (enough size for max intermediate results) */
#define IR_BUF_SZ_MOST (32 * 32 * 32) /*32768*/
#define IR_BUF_SZ_NEXT (32 * 16 * 16)
static d_type _Z x_mem_buf[IR_BUF_SZ_MOST];
static d_type _Y y_mem_buf[IR_BUF_SZ_NEXT];

/* Module Input/Output tensors and their's external interface */
static mli_tensor input = {
	.data = (void *)x_mem_buf,
	.capacity = sizeof(d_type) * IN_POINTS,
	.shape = { 32, 32, 3 },
	.rank = 3,
	.el_type = D_EL_TYPE,
	.el_params.fx.frac_bits = 7,
};

static mli_tensor output = {
	.data = (void *)y_mem_buf,
	.capacity = sizeof(d_type) * OUT_POINTS,
	.shape = { 10 },
	.rank = 1,
	.el_type = D_EL_TYPE,
	.el_params.fx.frac_bits = 0,
};

/* Interface variables: Available to user via main model header */
mli_tensor *const cifar10_cf_net_input = &input;
mli_tensor *const cifar10_cf_net_output = &output;

/* -----------------  Model description and configuration ---------------- */
/* Configuration objects for layers */

static const mli_permute_cfg permute_hwc2chw_cfg = { .perm_dim = { 2, 0, 1 } };

static const mli_conv2d_cfg shared_conv_cfg = { .stride_height = 1,
						.stride_width = 1,
						.padding_bottom = 2,
						.padding_top = 2,
						.padding_left = 2,
						.padding_right = 2,
						.relu.type = MLI_RELU_GEN };

static const mli_pool_cfg shared_pool_cfg = { .kernel_height = 3,
					      .kernel_width = 3,
					      .stride_height = 2,
					      .stride_width = 2,
					      .padding_bottom = 1,
					      .padding_top = 0,
					      .padding_left = 0,
					      .padding_right = 1 };

/* Conv 1 Layer related tensors */
static const mli_tensor L1_conv_wt = {
	.data = (void *)L1_conv_wt_buf,
	.capacity = CONV1_W_ELEMENTS * sizeof(w_type),
	.shape = CONV1_W_SHAPE,
	.rank = CONV1_W_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = CONV1_W_FRAQ,
};

static const mli_tensor L1_conv_bias = {
	.data = (void *)L1_conv_bias_buf,
	.capacity = CONV1_B_ELEMENTS * sizeof(w_type),
	.shape = CONV1_B_SHAPE,
	.rank = CONV1_B_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = CONV1_B_FRAQ,
};

/* Conv 2 Layer related data */
static mli_tensor L2_conv_wt = {
	.data = (void *)L2_conv_wt_buf,
	.capacity = CONV2_W_ELEMENTS * sizeof(w_type),
	.shape = CONV2_W_SHAPE,
	.rank = CONV2_W_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = CONV2_W_FRAQ,
};

static mli_tensor L2_conv_bias = {
	.data = (void *)L2_conv_bias_buf,
	.capacity = CONV2_B_ELEMENTS * sizeof(w_type),
	.shape = CONV2_B_SHAPE,
	.rank = CONV2_B_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = CONV2_B_FRAQ,
};

/* Conv 3 Layer related data */
static mli_tensor L3_conv_wt = {
	.data = (void *)L3_conv_wt_buf,
	.capacity = CONV3_W_ELEMENTS * sizeof(w_type),
	.shape = CONV3_W_SHAPE,
	.rank = CONV3_W_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = CONV3_W_FRAQ,
};

static mli_tensor L3_conv_bias = {
	.data = (void *)L3_conv_bias_buf,
	.capacity = CONV3_B_ELEMENTS * sizeof(w_type),
	.shape = CONV3_B_SHAPE,
	.rank = CONV3_B_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = CONV3_B_FRAQ,
};

/* FC4 Layer related data */
static mli_tensor L4_fc_wt = {
	.data = (void *)L4_fc_wt_buf,
	.capacity = FC4_W_ELEMENTS * sizeof(w_type),
	.shape = FC4_W_SHAPE,
	.rank = FC4_W_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = FC4_W_FRAQ,
};

static mli_tensor L4_fc_bias = {
	.data = (void *)L4_fc_bias_buf,
	.capacity = FC4_B_ELEMENTS * sizeof(w_type),
	.shape = FC4_B_SHAPE,
	.rank = FC4_B_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = FC4_B_FRAQ,
};

#if defined(MODEL_BIG)
/* FC5 Layer related data */
static mli_tensor L5_fc_wt = {
	.data = (void *)L5_fc_wt_buf,
	.capacity = FC5_W_ELEMENTS * sizeof(w_type),
	.shape = FC5_W_SHAPE,
	.rank = FC5_W_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = FC5_W_FRAQ,
};

static mli_tensor L5_fc_bias = {
	.data = (void *)L5_fc_bias_buf,
	.capacity = FC5_B_ELEMENTS * sizeof(w_type),
	.shape = FC5_B_SHAPE,
	.rank = FC5_B_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = FC5_B_FRAQ,
};
#endif /*#if defined(MODEL_BIG)*/

/* Intermediate result tensors */
static mli_tensor ir_tensor_X = {
	.data = (void *)x_mem_buf,
	.capacity = sizeof(x_mem_buf),
	.shape = { 0, 0, 0, 0 },
	.rank = 4,
	.el_type = D_EL_TYPE,
	.el_params.fx.frac_bits = FRQ_BITS(0, d_type),
};

static mli_tensor ir_tensor_Y = {
	.data = (void *)y_mem_buf,
	.capacity = sizeof(y_mem_buf),
	.shape = { 0, 0, 0, 0 },
	.rank = 4,
	.el_type = D_EL_TYPE,
	.el_params.fx.frac_bits = FRQ_BITS(0, d_type),
};

/*  Wrappers on MLI calls to deal with various */
/*  bit depth configurable in compile time */

static inline mli_status maxpool_chw(const mli_tensor *in, const mli_pool_cfg *cfg,
				     mli_tensor *out);

static inline mli_status avepool_chw(const mli_tensor *in, const mli_pool_cfg *cfg,
				     mli_tensor *out);

static inline mli_status softmax(const mli_tensor *in, mli_tensor *out);

static inline mli_status relu(const mli_tensor *in, const mli_relu_cfg *cfg, mli_tensor *out);

static inline mli_status mli_krn_permute_fx(const mli_tensor *in, const mli_permute_cfg *cfg,
					    mli_tensor *out);

static inline mli_status conv2d_chw(const mli_tensor *in, const mli_tensor *weights,
				    const mli_tensor *bias, const mli_conv2d_cfg *cfg,
				    mli_tensor *out);

static inline mli_status fully_connected(const mli_tensor *in, const mli_tensor *weights,
					 const mli_tensor *bias, mli_tensor *out);

/* ------------------- Check kernel result. Debug function ------------------ */

static void check_result(const char *ir_root, const char *ref_file, mli_tensor *pred_tsr,
			 unsigned int cycles, mli_status ret_code);

/* ------------------ CIFAR10 graph based on Caffe example. ----------------- */
/* ----------------- Layer-by-Layer execution for CHW layput ---------------- */

void cifar10_cf_net(const char *debug_ir_root)
{
	if (debug_ir_root == NULL) {
/* Version A: Pure implementation: without return status checking and profiling wrappers */
		/* LAYER 0: Change RGB Image layout */
		mli_krn_permute_fx(&input, &permute_hwc2chw_cfg, &ir_tensor_Y);

		/* --------------------------------- LAYER 1 -------------------------------- */
		ir_tensor_X.el_params.fx.frac_bits = CONV1_OUT_FRAQ;
		conv2d_chw(&ir_tensor_Y, &L1_conv_wt, &L1_conv_bias, &shared_conv_cfg,
			   &ir_tensor_X);
		maxpool_chw(&ir_tensor_X, &shared_pool_cfg, &ir_tensor_Y);

		/* --------------------------------- LAYER 2 -------------------------------- */
		ir_tensor_X.el_params.fx.frac_bits = CONV2_OUT_FRAQ;
		conv2d_chw(&ir_tensor_Y, &L2_conv_wt, &L2_conv_bias, &shared_conv_cfg,
			   &ir_tensor_X);
		avepool_chw(&ir_tensor_X, &shared_pool_cfg, &ir_tensor_Y);

		/* --------------------------------- LAYER 3 -------------------------------- */
		ir_tensor_X.el_params.fx.frac_bits = CONV3_OUT_FRAQ;
		conv2d_chw(&ir_tensor_Y, &L3_conv_wt, &L3_conv_bias, &shared_conv_cfg,
			   &ir_tensor_X);
		avepool_chw(&ir_tensor_X, &shared_pool_cfg, &ir_tensor_Y);

		/* --------------------------------- LAYER 4 -------------------------------- */
		ir_tensor_X.el_params.fx.frac_bits = FC4_OUT_FRAQ;
		fully_connected(&ir_tensor_Y, &L4_fc_wt, &L4_fc_bias, &ir_tensor_X);

#if defined(MODEL_BIG)
		/* --------------------------------- LAYER 5 -------------------------------- */
		ir_tensor_Y.el_params.fx.frac_bits = FC5_OUT_FRAQ;
		fully_connected(&ir_tensor_X, &L5_fc_wt, &L5_fc_bias, &ir_tensor_Y);
		softmax(&ir_tensor_Y, &output);
#else
		softmax(&ir_tensor_X, &output);
#endif

	} else {
	/* Version B: Wrapped by service code for profiling and IR results checking purpose */
		mli_status ret = MLI_STATUS_OK;
		unsigned int layer1_cycles = 0;
		unsigned int layer2_cycles = 0;
		unsigned int layer3_cycles = 0;
		unsigned int layer4_cycles = 0;
		unsigned int layer5_cycles = 0;

		/* ------------------------- Change RGB Image layout ------------------------ */
		PROFILE(ret = mli_krn_permute_fx(&input, &permute_hwc2chw_cfg, &ir_tensor_Y));
		check_result(debug_ir_root, "RGB_2_CHW", &ir_tensor_Y, cycle_cnt, ret);
		layer1_cycles += cycle_cnt;

		/* --------------------------------- LAYER 1 -------------------------------- */
		ir_tensor_X.el_params.fx.frac_bits = CONV1_OUT_FRAQ;
		PROFILE(ret = conv2d_chw(&ir_tensor_Y, &L1_conv_wt, &L1_conv_bias, &shared_conv_cfg,
					 &ir_tensor_X));
		check_result(debug_ir_root, "ir_conv1.idx", &ir_tensor_X, cycle_cnt, ret);
		layer1_cycles += cycle_cnt;

		PROFILE(ret = maxpool_chw(&ir_tensor_X, &shared_pool_cfg, &ir_tensor_Y));
		check_result(debug_ir_root, "ir_pool1.idx", &ir_tensor_Y, cycle_cnt, ret);
		layer1_cycles += cycle_cnt;

		/* --------------------------------- LAYER 2 -------------------------------- */
		ir_tensor_X.el_params.fx.frac_bits = CONV2_OUT_FRAQ;
		PROFILE(ret = conv2d_chw(&ir_tensor_Y, &L2_conv_wt, &L2_conv_bias, &shared_conv_cfg,
					 &ir_tensor_X));
		check_result(debug_ir_root, "ir_conv2.idx", &ir_tensor_X, cycle_cnt, ret);
		layer2_cycles += cycle_cnt;

		PROFILE(ret = avepool_chw(&ir_tensor_X, &shared_pool_cfg, &ir_tensor_Y));
		check_result(debug_ir_root, "ir_pool2.idx", &ir_tensor_Y, cycle_cnt, ret);
		layer2_cycles += cycle_cnt;

		/* --------------------------------- LAYER 3 -------------------------------- */
		ir_tensor_X.el_params.fx.frac_bits = CONV3_OUT_FRAQ;
		PROFILE(ret = conv2d_chw(&ir_tensor_Y, &L3_conv_wt, &L3_conv_bias, &shared_conv_cfg,
					 &ir_tensor_X));
		check_result(debug_ir_root, "ir_conv3.idx", &ir_tensor_X, cycle_cnt, ret);
		layer3_cycles += cycle_cnt;

		PROFILE(ret = avepool_chw(&ir_tensor_X, &shared_pool_cfg, &ir_tensor_Y));
		check_result(debug_ir_root, "ir_pool3.idx", &ir_tensor_Y, cycle_cnt, ret);
		layer3_cycles += cycle_cnt;

		/* --------------------------------- LAYER 4 -------------------------------- */
		ir_tensor_X.el_params.fx.frac_bits = FC4_OUT_FRAQ;
		PROFILE(ret = fully_connected(&ir_tensor_Y, &L4_fc_wt, &L4_fc_bias, &ir_tensor_X));
		check_result(debug_ir_root, "ir_ip1.idx", &ir_tensor_X, cycle_cnt, ret);
		layer4_cycles += cycle_cnt;

		/* --------------------------------- LAYER 5 -------------------------------- */
#if defined(MODEL_BIG)
		ir_tensor_Y.el_params.fx.frac_bits = FC5_OUT_FRAQ;
		PROFILE(ret = fully_connected(&ir_tensor_X, &L5_fc_wt, &L5_fc_bias, &ir_tensor_Y));
		check_result(debug_ir_root, "ir_ip2.idx", &ir_tensor_Y, cycle_cnt, ret);
		layer5_cycles += cycle_cnt;

		PROFILE(ret = softmax(&ir_tensor_Y, &output));
		check_result(debug_ir_root, "ir_prob.idx", &output, cycle_cnt, ret);
		layer5_cycles += cycle_cnt;
#else
		PROFILE(ret = softmax(&ir_tensor_X, &output));
		check_result(debug_ir_root, "ir_prob.idx", &output, cycle_cnt, ret);
		layer5_cycles += cycle_cnt;
#endif

		const unsigned int total = layer1_cycles + layer2_cycles + layer3_cycles +
					   layer4_cycles + layer5_cycles;
		printf("\n\nSummary:\n"
		       "\tLayer1: %u cycles\n"
		       "\tLayer2: %u cycles\n"
		       "\tLayer3: %u cycles\n"
		       "\tLayer4: %u cycles\n"
		       "\tLayer5: %u cycles\n"
		       "\n\tTotal: %u cycles\n\n",
		       layer1_cycles, layer2_cycles, layer3_cycles, layer4_cycles, layer5_cycles,
		       total);
	}
}

/* ----------------- Checking kernel result. Debug function ----------------- */

static void check_result(const char *ir_root, const char *ref_file, mli_tensor *pred_tsr,
			 unsigned int cycles, mli_status ret_code)
{
	if (ret_code != MLI_STATUS_OK) {
		printf("ERROR: MLI Code for %s (%d) is not OK\n", ref_file, ret_code);
		assert(0);
	}

	if (ir_root != NULL) {
		struct ref_to_pred_output err;
		enum test_status test_result = measure_ref_to_pred(ir_root, ref_file,
										*pred_tsr, &err);

		if (test_result == TEST_PASSED) {
			printf("%s:\n\tS/N=%-10.1f (%-4.1f db)\n\t%u cycles\n", ref_file,
			       err.ref_vec_length / err.noise_vec_length, err.ref_to_noise_snr,
			       cycles);
		} else if (test_result == TEST_FAILED) {
			printf("ERROR: Test suit returns FAILED code for %s\n", ref_file);
			assert(0);
		} else {
			printf("%s(w/o IR check):\t%u cycles\n", ref_file, cycles);
		}
	}
}

/* -------------------------------------------------------------------------- */
/*                 MLI Functions wrappers: Kernels w/o weights                */
/* -------------------------------------------------------------------------- */
#if (MODEL_BIT_DEPTH != MODEL_FX_8)
static inline mli_status maxpool_chw(const mli_tensor *in, const mli_pool_cfg *cfg, mli_tensor *out)
{
	return mli_krn_maxpool_chw_fx16_k3x3_krnpad(in, cfg, out);
}

static inline mli_status avepool_chw(const mli_tensor *in, const mli_pool_cfg *cfg, mli_tensor *out)
{
	return mli_krn_avepool_chw_fx16_k3x3_krnpad(in, cfg, out);
}

static inline mli_status softmax(const mli_tensor *in, mli_tensor *out)
{
	return mli_krn_softmax_fx16(in, out);
}

static inline mli_status relu(const mli_tensor *in, const mli_relu_cfg *cfg, mli_tensor *out)
{
	return mli_krn_relu_fx16(in, cfg, out);
}

static inline mli_status mli_krn_permute_fx(const mli_tensor *in, const mli_permute_cfg *cfg,
					    mli_tensor *out)
{
	return mli_krn_permute_fx16(in, cfg, out);
}

#else /* MODEL_BIT_DEPTH == (MODEL_FX_8W16D || MODEL_FX_8W16D) */
static inline mli_status maxpool_chw(const mli_tensor *in, const mli_pool_cfg *cfg, mli_tensor *out)
{
	return mli_krn_maxpool_chw_fx8_k3x3_krnpad(in, cfg, out);
}

static inline mli_status avepool_chw(const mli_tensor *in, const mli_pool_cfg *cfg, mli_tensor *out)
{
	return mli_krn_avepool_chw_fx8_k3x3_krnpad(in, cfg, out);
}

static inline mli_status softmax(const mli_tensor *in, mli_tensor *out)
{
	return mli_krn_softmax_fx8(in, out);
}

static inline mli_status relu(const mli_tensor *in, const mli_relu_cfg *cfg, mli_tensor *out)
{
	return mli_krn_relu_fx8(in, cfg, out);
}

static inline mli_status mli_krn_permute_fx(const mli_tensor *in, const mli_permute_cfg *cfg,
					    mli_tensor *out)
{
	return mli_krn_permute_fx8(in, cfg, out);
}

#endif

/* -------------------------------------------------------------------------- */
/*                MLI Functions wrappers: Kernels with weights                */
/* -------------------------------------------------------------------------- */

#if (MODEL_BIT_DEPTH == MODEL_FX_8)
static inline mli_status conv2d_chw(const mli_tensor *in, const mli_tensor *weights,
				    const mli_tensor *bias, const mli_conv2d_cfg *cfg,
				    mli_tensor *out)
{
	return mli_krn_conv2d_chw_fx8_k5x5_str1_krnpad(in, weights, bias, cfg, out);
}

static inline mli_status fully_connected(const mli_tensor *in, const mli_tensor *weights,
					 const mli_tensor *bias, mli_tensor *out)
{
	return mli_krn_fully_connected_fx8(in, weights, bias, out);
}

#elif (MODEL_BIT_DEPTH == MODEL_FX_16)
static inline mli_status conv2d_chw(const mli_tensor *in, const mli_tensor *weights,
				    const mli_tensor *bias, const mli_conv2d_cfg *cfg,
				    mli_tensor *out)
{
	return mli_krn_conv2d_chw_fx16_k5x5_str1_krnpad(in, weights, bias, cfg, out);
}

static inline mli_status fully_connected(const mli_tensor *in, const mli_tensor *weights,
					 const mli_tensor *bias, mli_tensor *out)
{
	return mli_krn_fully_connected_fx16(in, weights, bias, out);
}

#else /* MODEL_BIT_DEPTH == MODEL_FX_8W16D */
static inline mli_status conv2d_chw(const mli_tensor *in, const mli_tensor *weights,
				    const mli_tensor *bias, const mli_conv2d_cfg *cfg,
				    mli_tensor *out)
{
	return mli_krn_conv2d_chw_fx8w16d(in, weights, bias, cfg, out);
}

static inline mli_status fully_connected(const mli_tensor *in, const mli_tensor *weights,
					 const mli_tensor *bias, mli_tensor *out)
{
	return mli_krn_fully_connected_fx8w16d(in, weights, bias, out);
}
#endif
