/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "har_smartphone_model.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mli_api.h"
#include "mli_types.h"
#include "mli_config.h"
#include "har_smartphone_constants.h"
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
#define LSTM_CELL_SZ (32)
#define INOUT_BUF_SZ_MOST (128 * LSTM_CELL_SZ)
#define LSTM_IR_BUF_SZ (4 * LSTM_CELL_SZ)
#define LSTM_CELL_SZ (32)

/*
 * Despite the name of buf we keep all in/out data
 * in the same bank (typically first in operand)
 * Weights and lstm memory in the another (typically second input operand)
 * 11d has got only 2 separate banks of memory
 */
static d_type _Y x_mem_buf[INOUT_BUF_SZ_MOST];
static d_type _Y y_mem_buf[INOUT_BUF_SZ_MOST];
static d_type _Y lstm_ir_mem_buf[LSTM_IR_BUF_SZ];
static d_type _X lstm_cell_mem_buf[LSTM_CELL_SZ];

/* Module Input/Output tensors and their's external interface */
static mli_tensor input = {
	.data = (void *)x_mem_buf,
	.capacity = sizeof(d_type) * IN_POINTS,
	.shape = { 128, 9 },
	.rank = 2,
	.el_type = D_EL_TYPE,
	.el_params.fx.frac_bits = sizeof(d_type) * 8 - 1 - 2, /* FRAQ_BITS(0, d_type), */
};

static mli_tensor output = {
	.data = (void *)y_mem_buf,
	.capacity = sizeof(d_type) * OUT_POINTS,
	.shape = { 6 },
	.rank = 1,
	.el_type = D_EL_TYPE,
	.el_params.fx.frac_bits = 0,
};

/* Interface variables: Available to user via main model header */
mli_tensor *const har_smartphone_net_input = &input;
mli_tensor *const har_smartphone_net_output = &output;

/* -------------------------------------------------------------------------- */
/*                   //  Model description and configuration                  */
/* -------------------------------------------------------------------------- */

/* Intermediate and helper tensors */
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

static mli_tensor lstm_ir_tensor = {
	.data = (void *)lstm_ir_mem_buf,
	.capacity = sizeof(lstm_ir_mem_buf),
	.shape = { 0, 0, 0, 0 },
	.rank = 4,
	.el_type = D_EL_TYPE,
	.el_params.fx.frac_bits = FRQ_BITS(0, d_type),
};

static mli_tensor lstm_cell_tensor = {
	.data = lstm_cell_mem_buf,
	.capacity = sizeof(lstm_cell_mem_buf),
	.shape = { LSTM_CELL_SZ },
	.rank = 1,
	.el_type = D_EL_TYPE,
	.el_params.fx.frac_bits = 0, /* TO BE UPDATED BEFORE usage */
};

static mli_tensor lstm_prev_tensor = {
	.data = NULL, /* TO BE UPDATED BEFORE usage */
	.capacity = sizeof(lstm_cell_mem_buf),
	.shape = { LSTM_CELL_SZ },
	.rank = 1,
	.el_type = D_EL_TYPE,
	.el_params.fx.frac_bits = 0, /* TO BE UPDATED BEFORE usage */
};

/* Layer 1: Fully Connected related data */
static const mli_tensor L1_fc_wt = {
	.data = (void *)L1_fc_wt_buf,
	.capacity = FC1_W_ELEMENTS * sizeof(w_type),
	.shape = FC1_W_SHAPE,
	.rank = FC1_W_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = FC1_W_FRAQ,
};

static const mli_tensor L1_fc_bias = {
	.data = (void *)L1_fc_bias_buf,
	.capacity = FC1_B_ELEMENTS * sizeof(w_type),
	.shape = FC1_B_SHAPE,
	.rank = FC1_B_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = FC1_B_FRAQ,
};

static const mli_relu_cfg L1_relu_cfg = { .type = MLI_RELU_GEN };

/* LSTM Layer 2 related data */
static const mli_tensor L2_lstm_wt = {
	.data = (void *)L2_lstm_wt_buf,
	.capacity = LSTM2_W_ELEMENTS * sizeof(w_type),
	.shape = LSTM2_W_SHAPE,
	.rank = LSTM2_W_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = LSTM2_W_FRAQ,
};

static const mli_tensor L2_lstm_bias = {
	.data = (void *)L2_lstm_bias_buf,
	.capacity = LSTM2_B_ELEMENTS * sizeof(w_type),
	.shape = LSTM2_B_SHAPE,
	.rank = LSTM2_B_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = LSTM2_B_FRAQ,
};

static const mli_rnn_cell_cfg L2_lstm_cfg = {
	.mode = RNN_BATCH_TO_BATCH,
	.act = RNN_ACT_TANH,
	.ir_tsr = &lstm_ir_tensor,
};

/* LSTM Layer 3 related data */
static const mli_tensor L3_lstm_wt = {
	.data = (void *)L3_lstm_wt_buf,
	.capacity = LSTM3_W_ELEMENTS * sizeof(w_type),
	.shape = LSTM3_W_SHAPE,
	.rank = LSTM3_W_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = LSTM3_W_FRAQ,
};

static const mli_tensor L3_lstm_bias = {
	.data = (void *)L3_lstm_bias_buf,
	.capacity = LSTM3_B_ELEMENTS * sizeof(w_type),
	.shape = LSTM3_B_SHAPE,
	.rank = LSTM3_B_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = LSTM3_B_FRAQ,
};

static const mli_rnn_cell_cfg L3_lstm_cfg = {
	.mode = RNN_BATCH_TO_LAST,
	.act = RNN_ACT_TANH,
	.ir_tsr = &lstm_ir_tensor,
};

/* FC4 Layer related data */
static const mli_tensor L4_fc_wt = {
	.data = (void *)L4_fc_wt_buf,
	.capacity = FC4_W_ELEMENTS * sizeof(w_type),
	.shape = FC4_W_SHAPE,
	.rank = FC4_W_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = FC4_W_FRAQ,
};

static const mli_tensor L4_fc_bias = {
	.data = (void *)L4_fc_bias_buf,
	.capacity = FC4_B_ELEMENTS * sizeof(w_type),
	.shape = FC4_B_SHAPE,
	.rank = FC4_B_RANK,
	.el_type = W_EL_TYPE,
	.el_params.fx.frac_bits = FC4_B_FRAQ,
};

/*
 * Wrappers on MLI Lib calls declaration
 * Next functions calls mli_lib kernels for appropriate data types
 * (MODEL_BIT_DEPTH define)
 */
static inline mli_status nn_relu(const mli_tensor *in, const mli_relu_cfg *cfg, mli_tensor *out);

static inline mli_status nn_fully_connected(const mli_tensor *in, const mli_tensor *weights,
					    const mli_tensor *bias, mli_tensor *out);

static inline mli_status nn_lstm_cell(const mli_tensor *in, const mli_tensor *prev_out,
				      const mli_tensor *weights, const mli_tensor *bias,
				      const mli_rnn_cell_cfg *cfg, mli_tensor *cell,
				      mli_tensor *out);

#if defined(CUSTOM_USER_LSTM_LAYER3)
static inline mli_status nn_sigm(const mli_tensor *in, mli_tensor *out);

static inline mli_status nn_tanh(const mli_tensor *in, mli_tensor *out);

static inline mli_status nn_eltwise_mul(const mli_tensor *in1, const mli_tensor *in2,
					mli_tensor *out);

static inline mli_status nn_eltwise_add(const mli_tensor *in1, const mli_tensor *in2,
					mli_tensor *out);

static inline mli_status nn_rnn_cell(const mli_tensor *in, const mli_tensor *prev_out,
				     const mli_tensor *weights, const mli_tensor *bias,
				     const mli_rnn_cell_cfg *cfg, mli_tensor *out);
#endif

 /* -------------------------------------------------------------------------- */
 /*          Declaration of helper functions and user specific kernels         */
 /* -------------------------------------------------------------------------- */
static mli_status user_fc_on_multiple_samples(const mli_tensor *input, mli_tensor *output);

static mli_status user_lstm_batch_to_last(const mli_tensor *in, const mli_tensor *prev_out,
					  const mli_tensor *weights, const mli_tensor *bias,
					  const mli_rnn_cell_cfg *lstm_cfg, mli_tensor *cell,
					  mli_tensor *out);

static void check_result(const char *ir_root, const char *ref_file, mli_tensor *pred_tsr,
			 unsigned int cycles, mli_status ret_code);

 /* -------------------------------------------------------------------------- */
 /*            HAR Smartphone graph based. Layer-by-Layer execution            */
 /* -------------------------------------------------------------------------- */
void har_smartphone_net(const char *debug_ir_root)
{
	if (debug_ir_root == NULL) {
		/* Version A: without return status checking and profiling wrappers */

		/* LAYER 1 */
		ir_tensor_Y.el_params.fx.frac_bits = FC1_OUT_FRAQ;
		user_fc_on_multiple_samples(&input, &ir_tensor_Y);
		nn_relu(&ir_tensor_Y, &L1_relu_cfg, &ir_tensor_X);

		/* LAYER 2 */
		d_type *cell_ptr = (d_type *)lstm_cell_tensor.data;
		d_type *prev_out_ptr = (d_type *)ir_tensor_Y.data;

		for (int idx = 0; idx < LSTM_CELL_SZ; idx++)
			cell_ptr[idx] = prev_out_ptr[idx] = 0;

		/* Completion of state tensors description */
		lstm_prev_tensor.data = prev_out_ptr;
		lstm_prev_tensor.el_params.fx.frac_bits = LSTM2_OUT_FRAQ;
		lstm_cell_tensor.el_params.fx.frac_bits = LSTM2_CELL_FRAQ;

		nn_lstm_cell(&ir_tensor_X, &lstm_prev_tensor, &L2_lstm_wt, &L2_lstm_bias,
			     &L2_lstm_cfg, &lstm_cell_tensor, &ir_tensor_Y);

		/* LAYER 3 */
		cell_ptr = (d_type *)lstm_cell_tensor.data;
		prev_out_ptr = (d_type *)ir_tensor_X.data;
		for (int idx = 0; idx < LSTM_CELL_SZ; idx++)
			cell_ptr[idx] = prev_out_ptr[idx] = 0;

		/* Completion state tensors description */
		lstm_prev_tensor.data = prev_out_ptr;
		lstm_prev_tensor.el_params.fx.frac_bits = LSTM3_OUT_FRAQ;
		lstm_cell_tensor.el_params.fx.frac_bits = LSTM3_CELL_FRAQ;

		user_lstm_batch_to_last(&ir_tensor_Y, &lstm_prev_tensor, &L3_lstm_wt, &L3_lstm_bias,
					&L3_lstm_cfg, &lstm_cell_tensor, &ir_tensor_X);

		/* LAYER 4 */
		output.el_params.fx.frac_bits = FC4_OUT_FRAQ;
		nn_fully_connected(&ir_tensor_X, &L4_fc_wt, &L4_fc_bias, &output);
	} else {
	/* Version A: Wrapped by service code for profiling and IR results checking purpose */

		mli_status ret = MLI_STATUS_OK;
		unsigned int layer1_cycles = 0;
		unsigned int layer2_cycles = 0;
		unsigned int layer3_cycles = 0;
		unsigned int layer4_cycles = 0;

		/* LAYER 1 */
		ir_tensor_Y.el_params.fx.frac_bits = FC1_OUT_FRAQ;
		PROFILE(ret = user_fc_on_multiple_samples(&input, &ir_tensor_Y));
		check_result(debug_ir_root, "ir_fc1.idx", &ir_tensor_Y, cycle_cnt, ret);
		layer1_cycles += cycle_cnt;

		PROFILE(ret = nn_relu(&ir_tensor_Y, &L1_relu_cfg, &ir_tensor_X));
		check_result(debug_ir_root, "ir_relu1.idx", &ir_tensor_X, cycle_cnt, ret);
		layer1_cycles += cycle_cnt;

		/* LAYER 2 */
		/* Clear state buffers */
		d_type *cell_ptr = (d_type *)lstm_cell_tensor.data;
		d_type *prev_out_ptr = (d_type *)ir_tensor_Y.data;

		for (int idx = 0; idx < LSTM_CELL_SZ; idx++)
			cell_ptr[idx] = prev_out_ptr[idx] = 0;

		/* Completion state tensors description */
		lstm_prev_tensor.data = prev_out_ptr;
		lstm_prev_tensor.el_params.fx.frac_bits = LSTM2_OUT_FRAQ;
		lstm_cell_tensor.el_params.fx.frac_bits = LSTM2_CELL_FRAQ;

		PROFILE(ret = nn_lstm_cell(&ir_tensor_X, &lstm_prev_tensor, &L2_lstm_wt,
					   &L2_lstm_bias, &L2_lstm_cfg, &lstm_cell_tensor,
					   &ir_tensor_Y));
		check_result(debug_ir_root, "ir_lstm2.idx", &ir_tensor_Y, cycle_cnt, ret);
		layer2_cycles += cycle_cnt;

		/* LAYER 3 */
		cell_ptr = (d_type *)lstm_cell_tensor.data;
		prev_out_ptr = (d_type *)ir_tensor_X.data;
		for (int idx = 0; idx < LSTM_CELL_SZ; idx++)
			cell_ptr[idx] = prev_out_ptr[idx] = 0;

		/* Completion state tensors description */
		lstm_prev_tensor.data = prev_out_ptr;
		lstm_prev_tensor.el_params.fx.frac_bits = LSTM3_OUT_FRAQ;
		lstm_cell_tensor.el_params.fx.frac_bits = LSTM3_CELL_FRAQ;

		PROFILE(ret = user_lstm_batch_to_last(&ir_tensor_Y, &lstm_prev_tensor, &L3_lstm_wt,
						      &L3_lstm_bias, &L3_lstm_cfg,
						      &lstm_cell_tensor, &ir_tensor_X));
		check_result(debug_ir_root, "ir_lstm3.idx", &ir_tensor_X, cycle_cnt, ret);
		layer3_cycles += cycle_cnt;

		/* LAYER 4 */
		output.el_params.fx.frac_bits = FC4_OUT_FRAQ;
		PROFILE(ret = nn_fully_connected(&ir_tensor_X, &L4_fc_wt, &L4_fc_bias, &output));
		check_result(debug_ir_root, "ir_fc4.idx", &output, cycle_cnt, ret);
		layer4_cycles += cycle_cnt;

		const unsigned int total =
			layer1_cycles + layer2_cycles + layer3_cycles + layer4_cycles;
		printf("\n\nSummary:\n"
		       "\tLayer1: %u cycles\n"
		       "\tLayer2: %u cycles\n"
		       "\tLayer3: %u cycles\n"
		       "\tLayer4: %u cycles\n"
		       "\n\tTotal: %u cycles\n\n",
		       layer1_cycles, layer2_cycles, layer3_cycles, layer4_cycles, total);
	}
}

/*  Fully connected on batch: User Implementatioon */
static mli_status user_fc_on_multiple_samples(const mli_tensor *layer_input,
					      mli_tensor *layer_output)
{
	mli_status ret_val = MLI_STATUS_OK;
	mli_tensor fc1_in = { .rank = 1, .shape = { 0 } };
	mli_tensor fc1_out = { .data = layer_output->data,
			       .capacity = layer_output->capacity,
			       .el_params.fx.frac_bits = layer_output->el_params.fx.frac_bits };
	mli_point_to_subtsr_cfg iterator = { .start_coord = { 0 },
					     .coord_num = 1,
					     .first_out_dim_size = 1 };
	ret_val = mli_hlp_point_to_subtensor(layer_input, &iterator, &fc1_in);
	if (ret_val != MLI_STATUS_OK)
		return ret_val;

	unsigned int next_out_add =
		mli_hlp_count_elem_num(&L1_fc_bias, 0) * mli_hlp_tensor_element_size(&fc1_in);
	unsigned int next_in_add = fc1_in.capacity;

	for (int batch_idx = 0; batch_idx < layer_input->shape[0]; batch_idx++) {
		ret_val = nn_fully_connected(&fc1_in, &L1_fc_wt, &L1_fc_bias, &fc1_out);
		if (ret_val != MLI_STATUS_OK)
			return ret_val;
		fc1_in.data = next_in_add + (uint8_t *)fc1_in.data;
		fc1_out.data = next_out_add + (uint8_t *)fc1_out.data;
		fc1_out.capacity -= next_out_add;
	}

	layer_output->rank = 2;
	layer_output->shape[0] = layer_input->shape[0];
	layer_output->shape[1] = fc1_out.shape[0];
	layer_output->el_type = fc1_out.el_type;
	layer_output->el_params.fx.frac_bits = fc1_out.el_params.fx.frac_bits;

	return ret_val;
}

 /* -------------------------------------------------------------------------- */
 /*        User Implementatioon of LSTM cell through other MLI Kernels.        */
 /* -------------------------------------------------------------------------- */
static mli_status user_lstm_batch_to_last(const mli_tensor *in, const mli_tensor *prev_out,
					  const mli_tensor *weights, const mli_tensor *bias,
					  const mli_rnn_cell_cfg *lstm_cfg, mli_tensor *cell,
					  mli_tensor *out)
{
#if !defined(CUSTOM_USER_LSTM_LAYER3)
	/* Might be replaced with MLI function */
	return nn_lstm_cell(in, prev_out, weights, bias, lstm_cfg, cell, out);
#else
	mli_status ret_val = MLI_STATUS_OK;

	mli_rnn_cell_cfg rnn_cfg = { .act = RNN_ACT_NONE, .mode = RNN_ONE_TO_ONE, .ir_tsr = NULL };
	const mli_tensor *rnn_prev = prev_out;
	mli_tensor *ir_tensor = lstm_cfg->ir_tsr;

	/* 3 Int Bits for Non-Linearity input is enough */
	ir_tensor->el_params.fx.frac_bits = sizeof(d_type) * 8 - 1 - 3;

	/* Various gates to control info flow. */
	mli_tensor in_gate = { 0 }, g_tsr = { 0 }, forget_gate = { 0 }, out_gate = { 0 };
	mli_tensor new_g = { .data = out->data,
			     .capacity = out->capacity,
			     .el_params.fx.frac_bits = cell->el_params.fx.frac_bits };

	/* Iteration 0: Started outside of main cycle for initialization purpose */
	/* Step 1: Fully connected */
	mli_tensor rnn_in;
	mli_point_to_subtsr_cfg iterator = { .start_coord = { 0 },
					     .coord_num = 1,
					     .first_out_dim_size = 1 };
	ret_val = mli_hlp_point_to_subtensor(in, &iterator, &rnn_in);
	if (ret_val != MLI_STATUS_OK)
		return ret_val;

	ret_val = nn_rnn_cell(&rnn_in, rnn_prev, weights, bias, &rnn_cfg, ir_tensor);
	if (ret_val != MLI_STATUS_OK)
		return ret_val;

	rnn_prev = out;
	unsigned int next_in_add =
		mli_hlp_count_elem_num(&rnn_in, 0) * mli_hlp_tensor_element_size(&rnn_in);

	/* Init subtensors (current iterators state is suitable for it) */
	mli_hlp_point_to_subtensor(ir_tensor, &iterator, &in_gate);
	iterator.start_coord[0]++;
	mli_hlp_point_to_subtensor(ir_tensor, &iterator, &g_tsr);
	iterator.start_coord[0]++;
	mli_hlp_point_to_subtensor(ir_tensor, &iterator, &forget_gate);
	iterator.start_coord[0]++;
	mli_hlp_point_to_subtensor(ir_tensor, &iterator, &out_gate);

	/* Manual reshape */
	in_gate.shape[0] = g_tsr.shape[0] = forget_gate.shape[0] = out_gate.shape[0] =
		mli_hlp_count_elem_num(&in_gate, 0);
	in_gate.rank = g_tsr.rank = forget_gate.rank = out_gate.rank = 1;

	/* Iteration 0.3-127: outside of main cycle for initialization purpose */
	for (int batch_idx = 0; batch_idx < in->shape[0]; batch_idx++) {
		/* Step 2: Applying non-linearity */
		ret_val = nn_sigm(&in_gate, &in_gate);
		if (ret_val == MLI_STATUS_OK)
			ret_val = nn_tanh(&g_tsr, &g_tsr);
		if (ret_val == MLI_STATUS_OK)
			ret_val = nn_sigm(&forget_gate, &forget_gate);
		if (ret_val == MLI_STATUS_OK)
			ret_val = nn_sigm(&out_gate, &out_gate);
		if (ret_val != MLI_STATUS_OK)
			return ret_val;

		/* Step 3: Pointwise operations */
		ret_val = nn_eltwise_mul(&forget_gate, cell, cell);
		if (ret_val == MLI_STATUS_OK)
			ret_val = nn_eltwise_mul(&in_gate, &g_tsr, &new_g);
		if (ret_val == MLI_STATUS_OK)
			ret_val = nn_eltwise_add(cell, &new_g, cell);
		if (ret_val != MLI_STATUS_OK)
			return ret_val;

		/* Step 4: Calculate next output */
		ret_val = nn_tanh(cell, out);
		if (ret_val == MLI_STATUS_OK)
			ret_val = nn_eltwise_mul(out, &out_gate, out);
		if (ret_val != MLI_STATUS_OK)
			return ret_val;

		/* Next sample: Step 1: Fully connected */
		if (batch_idx < in->shape[0] - 1) {
			rnn_in.data = next_in_add + (uint8_t *)rnn_in.data;
			ret_val =
				nn_rnn_cell(&rnn_in, rnn_prev, weights, bias, &rnn_cfg, ir_tensor);
			if (ret_val != MLI_STATUS_OK)
				return ret_val;

/* Gate tensors point to RNN cell result, but structures have changed due to non-linearity. */
			/* Restore element params. */
			in_gate.el_params.fx.frac_bits = forget_gate.el_params.fx.frac_bits =
				g_tsr.el_params.fx.frac_bits = out_gate.el_params.fx.frac_bits =
					ir_tensor->el_params.fx.frac_bits;
		}
	}

	return ret_val;
#endif
}

 /* -------------------------------------------------------------------------- */
 /*                   Checking kernel result. Debug function                   */
 /* -------------------------------------------------------------------------- */
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
		} else
			printf("%s(w/o IR check):\t%u cycles\n", ref_file, cycles);
	}
}

 /* -------------------------------------------------------------------------- */
 /*                 MLI Functions wrappers: Kernels w/o weights                */
 /* -------------------------------------------------------------------------- */
#if (MODEL_BIT_DEPTH != MODEL_FX_8)
static inline mli_status nn_relu(const mli_tensor *in, const mli_relu_cfg *cfg, mli_tensor *out)
{
	return mli_krn_relu_fx16(in, cfg, out);
}

#if defined(CUSTOM_USER_LSTM_LAYER3)
static inline mli_status nn_sigm(const mli_tensor *in, mli_tensor *out)
{
	return mli_krn_sigm_fx16(in, out);
}

static inline mli_status nn_tanh(const mli_tensor *in, mli_tensor *out)
{
	return mli_krn_tanh_fx16(in, out);
}

static inline mli_status nn_eltwise_mul(const mli_tensor *in1, const mli_tensor *in2,
					mli_tensor *out)
{
	return mli_krn_eltwise_mul_fx16(in1, in2, out);
}

static inline mli_status nn_eltwise_add(const mli_tensor *in1, const mli_tensor *in2,
					mli_tensor *out)
{
	return mli_krn_eltwise_add_fx16(in1, in2, out);
}
#endif

#else /* MODEL_BIT_DEPTH == (MODEL_FX_8W16D || MODEL_FX_8W16D) */
static inline mli_status nn_relu(const mli_tensor *in, const mli_relu_cfg *cfg, mli_tensor *out)
{
	return mli_krn_relu_fx8(in, cfg, out);
}

#if defined(CUSTOM_USER_LSTM_LAYER3)
static inline mli_status nn_sigm(const mli_tensor *in, mli_tensor *out)
{
	return mli_krn_sigm_fx8(in, out);
}

static inline mli_status nn_tanh(const mli_tensor *in, mli_tensor *out)
{
	return mli_krn_tanh_fx8(in, out);
}

static inline mli_status nn_eltwise_mul(const mli_tensor *in1, const mli_tensor *in2,
					mli_tensor *out)
{
	return mli_krn_eltwise_mul_fx8(in1, in2, out);
}

static inline mli_status nn_eltwise_add(const mli_tensor *in1, const mli_tensor *in2,
					mli_tensor *out)
{
	return mli_krn_eltwise_add_fx8(in1, in2, out);
}
#endif

#endif /* MODEL_BIT_DEPTH */

 /* -------------------------------------------------------------------------- */
 /*                MLI Functions wrappers: Kernels with weights                */
 /* -------------------------------------------------------------------------- */
#if (MODEL_BIT_DEPTH == MODEL_FX_8)
static inline mli_status nn_fully_connected(const mli_tensor *in, const mli_tensor *weights,
					    const mli_tensor *bias, mli_tensor *out)
{
	return mli_krn_fully_connected_fx8(in, weights, bias, out);
}

static inline mli_status nn_lstm_cell(const mli_tensor *in, const mli_tensor *prev_out,
				      const mli_tensor *weights, const mli_tensor *bias,
				      const mli_rnn_cell_cfg *cfg, mli_tensor *cell,
				      mli_tensor *out)
{
	return mli_krn_lstm_cell_fx8(in, prev_out, weights, bias, cfg, cell, out);
}

#if defined(CUSTOM_USER_LSTM_LAYER3)
static inline mli_status nn_rnn_cell(const mli_tensor *in, const mli_tensor *prev_out,
				     const mli_tensor *weights, const mli_tensor *bias,
				     const mli_rnn_cell_cfg *cfg, mli_tensor *out)
{
	return mli_krn_basic_rnn_cell_fx8(in, prev_out, weights, bias, cfg, out);
}
#endif

#elif (MODEL_BIT_DEPTH == MODEL_FX_16)
static inline mli_status nn_fully_connected(const mli_tensor *in, const mli_tensor *weights,
					    const mli_tensor *bias, mli_tensor *out)
{
	return mli_krn_fully_connected_fx16(in, weights, bias, out);
}

static inline mli_status nn_lstm_cell(const mli_tensor *in, const mli_tensor *prev_out,
				      const mli_tensor *weights, const mli_tensor *bias,
				      const mli_rnn_cell_cfg *cfg, mli_tensor *cell,
				      mli_tensor *out)
{
	return mli_krn_lstm_cell_fx16(in, prev_out, weights, bias, cfg, cell, out);
}

#if defined(CUSTOM_USER_LSTM_LAYER3)
static inline mli_status nn_rnn_cell(const mli_tensor *in, const mli_tensor *prev_out,
				     const mli_tensor *weights, const mli_tensor *bias,
				     const mli_rnn_cell_cfg *cfg, mli_tensor *out)
{
	return mli_krn_basic_rnn_cell_fx16(in, prev_out, weights, bias, cfg, out);
}
#endif

#else /* MODEL_BIT_DEPTH == MODEL_FX_8W16D */
static inline mli_status nn_fully_connected(const mli_tensor *in, const mli_tensor *weights,
					    const mli_tensor *bias, mli_tensor *out)
{
	return mli_krn_fully_connected_fx8w16d(in, weights, bias, out);
}

static inline mli_status nn_lstm_cell(const mli_tensor *in, const mli_tensor *prev_out,
				      const mli_tensor *weights, const mli_tensor *bias,
				      const mli_rnn_cell_cfg *cfg, mli_tensor *cell,
				      mli_tensor *out)
{
	return mli_krn_lstm_cell_fx8w16d(in, prev_out, weights, bias, cfg, cell, out);
}

#if defined(CUSTOM_USER_LSTM_LAYER3)
static inline mli_status nn_rnn_cell(const mli_tensor *in, const mli_tensor *prev_out,
				     const mli_tensor *weights, const mli_tensor *bias,
				     const mli_rnn_cell_cfg *cfg, mli_tensor *out)
{
	return mli_krn_basic_rnn_cell_fx8w16d(in, prev_out, weights, bias, cfg, out);
}
#endif
#endif /* if (MODEL_BIT_DEPTH == *) */
