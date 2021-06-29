/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/
#include "tensor_transform.h"

#include <stdint.h>

#include "mli_api.h"
#include "tests_aux.h"

/* Transform float array to MLI FX tensor according to it's element type parameters */
mli_status mli_hlp_float_to_fx_tensor(const float *src, uint32_t src_size, mli_tensor *dst)
{
	mli_status ret = MLI_STATUS_OK;
	float scale_val = (float)((int64_t)1l << mli_hlp_tensor_scale_shift(dst)) /
			  (float)mli_hlp_tensor_scale(dst, 0);
	int16_t zero_offset = mli_hlp_tensor_zero_offset(dst, 0);

	if (dst->el_type == MLI_EL_FX_16) {
		if (dst->capacity < src_size * sizeof(int16_t))
			return MLI_STATUS_LENGTH_ERROR;

		int16_t *dst_arr = dst->data;

		for (int idx = 0; idx < src_size; idx++) {
			const float round_val = (src[idx] > 0) ? 0.5f : -0.5f;
			int32_t dst_val = (int32_t)(scale_val * src[idx] + round_val);

			dst_arr[idx] = (int16_t)(MIN(MAX(dst_val, INT16_MIN), INT16_MAX));
		}
	} else if (dst->el_type == MLI_EL_FX_8) {
		if (dst->capacity < src_size * sizeof(int8_t))
			return MLI_STATUS_LENGTH_ERROR;

		int8_t *dst_arr = dst->data;

		for (int idx = 0; idx < src_size; idx++) {
			const float round_val = (src[idx] > 0) ? 0.5f : -0.5f;

			const int32_t dst_val = (int32_t)(scale_val * src[idx] + round_val);

			dst_arr[idx] = (int8_t)(MIN(MAX(dst_val, INT8_MIN), INT8_MAX));
		}
	} else if (dst->el_type == MLI_EL_ASYM_I8) {
		if (dst->capacity < src_size * sizeof(int8_t))
			return MLI_STATUS_LENGTH_ERROR;

		int8_t *dst_arr = dst->data;

		if (dst->el_params.asym.dim < 0) {
			for (int idx = 0; idx < src_size; idx++) {
				const float round_val = (src[idx] > 0) ? 0.5f : -0.5f;
				const int32_t dst_val = (int32_t)(scale_val * src[idx] + round_val);

				dst_arr[idx] = (int8_t)(MIN(MAX(dst_val + zero_offset, INT8_MIN),
							    INT8_MAX));
			}
		} else {
			const int concat_dim = dst->el_params.asym.dim;
			const int total_elements = mli_hlp_count_elem_num(dst, 0);
			const int elements_to_convert =
				(concat_dim + 1 == dst->rank) ?
					      1 :
					      mli_hlp_count_elem_num(dst, concat_dim + 1);
			const int step_after_conv = mli_hlp_count_elem_num(dst, concat_dim);
			const int conversions_num = dst->shape[concat_dim];

			if (total_elements != src_size)
				return MLI_STATUS_LENGTH_ERROR;

			for (int c_idx = 0; c_idx < conversions_num; c_idx++) {
				scale_val = ((int64_t)1u << mli_hlp_tensor_scale_shift(dst)) /
					    (float)mli_hlp_tensor_scale(dst, (uint32_t)c_idx);
				zero_offset = mli_hlp_tensor_zero_offset(dst, c_idx);
				for (int data_idx = c_idx * elements_to_convert;
				     data_idx < total_elements; data_idx += step_after_conv) {
					for (int el_idx = 0; el_idx < elements_to_convert;
					     ++el_idx) {
						const float round_val =
							(src[data_idx + el_idx] > 0) ? 0.5f : -0.5f;
						int32_t dst_val =
							(int32_t)(scale_val *
									  src[data_idx + el_idx] +
								  round_val);
						dst_arr[data_idx + el_idx] = (int8_t)(MIN(
							MAX(dst_val + zero_offset, INT8_MIN),
							INT8_MAX));
					}
				}
			}
		}
	} else if (dst->el_type == MLI_EL_ASYM_I32) {
		if (dst->capacity < src_size * sizeof(int32_t))
			return MLI_STATUS_LENGTH_ERROR;

		int32_t *dst_arr = dst->data;

		if (dst->el_params.asym.dim < 0) {
			for (int idx = 0; idx < src_size; idx++) {
				const float round_val = (src[idx] > 0) ? 0.5f : -0.5f;
				int32_t dst_val = (int32_t)(scale_val * src[idx] + round_val);

				dst_arr[idx] = dst_val + zero_offset;
			}
		} else {
			const int concat_dim = dst->el_params.asym.dim;
			const int total_elements = mli_hlp_count_elem_num(dst, 0);
			const int elements_to_convert =
				(concat_dim + 1 == dst->rank) ?
					      1 :
					      mli_hlp_count_elem_num(dst, concat_dim + 1);
			const int step_after_conv = mli_hlp_count_elem_num(dst, concat_dim);
			const int conversions_num = dst->shape[concat_dim];

			if (total_elements != src_size)
				return MLI_STATUS_LENGTH_ERROR;

			for (int c_idx = 0; c_idx < conversions_num; c_idx++) {
				scale_val = ((int64_t)1u << mli_hlp_tensor_scale_shift(dst)) /
					    (float)mli_hlp_tensor_scale(dst, (uint32_t)c_idx);
				zero_offset = mli_hlp_tensor_zero_offset(dst, c_idx);
				for (int data_idx = c_idx * elements_to_convert;
				     data_idx < total_elements; data_idx += step_after_conv) {
					for (int el_idx = 0; el_idx < elements_to_convert;
					     ++el_idx) {
						const float round_val =
							(src[data_idx + el_idx] > 0) ? 0.5f : -0.5f;
						int32_t dst_val =
							(int32_t)(scale_val *
									  src[data_idx + el_idx] +
								  round_val);
						dst_arr[data_idx + el_idx] = dst_val + zero_offset;
					}
				}
			}
		}
	} else {
		ret = MLI_STATUS_TYPE_MISMATCH;
	}
	return ret;
}

/* -------------------------------------------------------------------------- */
/*                   Transform MLI FX tensor to float array                   */
/* -------------------------------------------------------------------------- */
mli_status mli_hlp_fx_tensor_to_float(const mli_tensor *src, float *dst, uint32_t dst_size)
{
	uint32_t elem_num = mli_hlp_count_elem_num(src, 0);

	if (elem_num > dst_size)
		return MLI_STATUS_LENGTH_ERROR;
	if (elem_num == 0)
		return MLI_STATUS_BAD_TENSOR;

	float scale_val = (float)mli_hlp_tensor_scale(src, 0) /
			  (float)((int64_t)1l << mli_hlp_tensor_scale_shift(src));
	int16_t zero_offset = mli_hlp_tensor_zero_offset(src, 0);

	if (src->el_type == MLI_EL_FX_16) {
		int16_t *src_arr = src->data;

		for (int idx = 0; idx < elem_num; idx++)
			dst[idx] = (float)(scale_val * src_arr[idx]);
	} else if (src->el_type == MLI_EL_FX_8) {
		int8_t *src_arr = src->data;

		for (int idx = 0; idx < elem_num; idx++)
			dst[idx] = (float)(scale_val * (src_arr[idx] - zero_offset));
	} else if (src->el_type == MLI_EL_ASYM_I8) {
		int8_t *src_arr = src->data;

		if (src->el_params.asym.dim < 0) {
			for (int idx = 0; idx < elem_num; idx++)
				dst[idx] = (float)(scale_val * (src_arr[idx] - zero_offset));
		} else {
			const int concat_dim = src->el_params.asym.dim;
			const int total_elements = mli_hlp_count_elem_num(src, 0);
			const int elements_to_convert =
				(concat_dim + 1 == src->rank) ?
					      1 :
					      mli_hlp_count_elem_num(src, concat_dim + 1);
			const int step_after_conv = mli_hlp_count_elem_num(src, concat_dim);
			const int conversions_num = src->shape[concat_dim];

			for (int c_idx = 0; c_idx < conversions_num; c_idx++) {
				scale_val = (float)mli_hlp_tensor_scale(src, c_idx) /
					    (float)((int64_t)1l << mli_hlp_tensor_scale_shift(src));
				zero_offset = mli_hlp_tensor_zero_offset(src, c_idx);
				for (int data_idx = c_idx * elements_to_convert;
				     data_idx < total_elements; data_idx += step_after_conv) {
					for (int el_idx = 0; el_idx < elements_to_convert;
					     ++el_idx) {
						dst[data_idx + el_idx] =
							(float)(scale_val *
								(src_arr[data_idx + el_idx] -
								 zero_offset));
					}
				}
			}
		}
	} else if (src->el_type == MLI_EL_ASYM_I32) {
		int32_t *src_arr = src->data;

		if (src->el_params.asym.dim < 0) {
			for (int idx = 0; idx < elem_num; idx++)
				dst[idx] = (float)(scale_val * (src_arr[idx] - zero_offset));
		} else {
			const int concat_dim = src->el_params.asym.dim;
			const int total_elements = mli_hlp_count_elem_num(src, 0);
			const int elements_to_convert =
				(concat_dim + 1 == src->rank) ?
					      1 :
					      mli_hlp_count_elem_num(src, concat_dim + 1);
			const int step_after_conv = mli_hlp_count_elem_num(src, concat_dim);
			const int conversions_num = src->shape[concat_dim];

			for (int c_idx = 0; c_idx < conversions_num; c_idx++) {
				scale_val = (float)mli_hlp_tensor_scale(src, c_idx) /
					    (float)((int64_t)1l << mli_hlp_tensor_scale_shift(src));
				zero_offset = mli_hlp_tensor_zero_offset(src, c_idx);
				for (int data_idx = c_idx * elements_to_convert;
				     data_idx < total_elements; data_idx += step_after_conv) {
					for (int el_idx = 0; el_idx < elements_to_convert;
					     ++el_idx) {
						dst[data_idx + el_idx] =
							(float)(scale_val *
								(src_arr[data_idx + el_idx] -
								 zero_offset));
					}
				}
			}
		}
	} else {
		return MLI_STATUS_TYPE_MISMATCH;
	}
	return MLI_STATUS_OK;
}
