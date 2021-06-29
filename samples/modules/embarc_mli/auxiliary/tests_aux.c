/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/
#include "tests_aux.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "mli_api.h"
#include "idx_file.h"
#include "tensor_transform.h"

// Assert wrapper: works only in DBG_MODE_FULL and DBG_MODE_DEBUG
#if defined(DEBUG)
#include <assert.h>
#define DEBUG_BREAK assert(0)
#else
#define DEBUG_BREAK (void)(0)
#endif

static const uint32_t kMaxBufSizeToMalloc = 32768;
static const uint32_t kMinBufSizeToMalloc = 32;

unsigned cycle_cnt = 0u;

const char *test_status_to_str[] = {
        "PASSED",
        "SKIPPED",
        "NOT_ENOUGH_MEM_ERR",
        "SUIT_ERROR",
        "FAILED"
};

//================================================================================
// Load several tensors from IDX files
//=================================================================================
test_status load_tensors_from_idx_files(
        const char * const test_root,
        const char * const paths[],
        mli_tensor * tensors[],
        uint32_t paths_num) {
    float * data = NULL;
    uint32_t data_buf_size = kMaxBufSizeToMalloc; // Memory allocated for source data
    tIdxDescr descr = {0, 0, 0, NULL};
    test_status ret = TEST_PASSED;
    const uint32_t max_path_sz = 128;
    char * path = NULL;

    // Step 1: Try to allocate data for path
    //=========================================
    path = malloc(max_path_sz);
    while (data == NULL && data_buf_size > kMinBufSizeToMalloc) {
        data = malloc(data_buf_size * sizeof(float));
        data_buf_size = (data == NULL)? data_buf_size >> 1: data_buf_size;
    }

    if (path == NULL || data == NULL) {
        DEBUG_BREAK;
        ret = TEST_NOT_ENOUGH_MEM;
        goto ret_label;
    }
    memset(path, 0, max_path_sz);

    // Step 2: Main reading cycle for each tensor in list
    //===================================================
    // for each vector we must read data by small parts
    // (to not allocate huge piece of memory)
    for (uint32_t idx = 0; idx < paths_num; idx++) {
        const uint32_t elem_size = mli_hlp_tensor_element_size(tensors[idx]);
        if (elem_size == 0) {
            DEBUG_BREAK;
            ret =  TEST_SUIT_ERROR;
            goto ret_label;
        }

        // Construct path and init helper data
        sprintf(path, "%s/%s", test_root, paths[idx]);

        // Step 2: Open file and quic check of content;
        //=============================================
        if ((descr.opened_file = fopen(path, "rb")) == NULL ||
                (idx_file_check_and_get_info(&descr)) != IDX_ERR_NONE ||
                descr.data_type != IDX_DT_FLOAT_4B ||
                descr.num_dim > MLI_MAX_RANK) {
            DEBUG_BREAK;
            ret =  TEST_SUIT_ERROR;
            goto ret_label;
        }

        // Step 3: Check memory requirements and read shape;
        //====================================
        if (elem_size * descr.num_elements > tensors[idx]->capacity) {
            DEBUG_BREAK;
            ret =  TEST_NOT_ENOUGH_MEM;
            goto ret_label;
        }

        uint32_t total_elements = descr.num_elements;
        descr.num_elements = 0;
        tensors[idx]->rank = descr.num_dim;
        if (idx_file_read_data(&descr, NULL, tensors[idx]->shape) != IDX_ERR_NONE) {
            DEBUG_BREAK;
            ret =  TEST_SUIT_ERROR;
            goto ret_label;
        }

        // Step 4: Read data by parts;
        //====================================
        uint32_t elements_accounted = 0;
        void *addr_backup = tensors[idx]->data;
        while (elements_accounted  < total_elements) {
            descr.num_elements =
                    ((total_elements - elements_accounted ) < data_buf_size) ?
                    (total_elements - elements_accounted ):
                    data_buf_size;

            if ( idx_file_read_data(&descr, (void *)data, NULL) != IDX_ERR_NONE ||
                    mli_hlp_float_to_fx_tensor(data, descr.num_elements, tensors[idx]) != MLI_STATUS_OK) {
                DEBUG_BREAK;
                ret =  TEST_SUIT_ERROR;
                tensors[idx]->data = addr_backup;
                goto ret_label;
            }

            elements_accounted += descr.num_elements;
            tensors[idx]->data += descr.num_elements * elem_size;
        }

        tensors[idx]->data = addr_backup;
        tensors[idx]->capacity = total_elements * elem_size;
        fclose(descr.opened_file);
        descr.opened_file = NULL;
    }

ret_label:
    if (descr.opened_file != NULL)
        fclose(descr.opened_file);
    if (path != NULL)
        free(path);
    if (data != NULL)
        free(data);
    return ret;
}


//================================================================================
// Compare pred tensor with reference one stored in external IDX file
//=================================================================================
test_status measure_ref_to_pred(
        const char * const tests_root,
        const char * const ref_vec_file,
        mli_tensor pred,
        ref_to_pred_output *out) {
    float * data = NULL;
    float * ref_buf = NULL;
    float * pred_buf = NULL;
    uint32_t data_buf_size = kMaxBufSizeToMalloc;
    test_status ret = TEST_PASSED;
    const uint32_t max_path_sz = 128;
    const uint32_t buffers_to_alloc = 2;
    char * path = NULL;

    // Step 1: Try to allocate data for path
    //=========================================
    const uint32_t pred_elem_size = mli_hlp_tensor_element_size(&pred);
    if (pred_elem_size == 0) {
        DEBUG_BREAK;
        ret =  TEST_SUIT_ERROR;
        goto ret_label;
    }

    path = malloc(max_path_sz);
    while (data == NULL && data_buf_size > kMinBufSizeToMalloc) {
        data = malloc(data_buf_size * buffers_to_alloc * sizeof(float));
        data_buf_size = (data == NULL)? data_buf_size >> 1: data_buf_size;
    }
    if (path == NULL || data == NULL) {
        DEBUG_BREAK;
        ret = TEST_NOT_ENOUGH_MEM;
        goto ret_label;
    }
    memset(path, 0, max_path_sz);
    sprintf(path, "%s/%s", tests_root, ref_vec_file);
    ref_buf = data;
    pred_buf = &data[data_buf_size];

    // Step 2: Open reference file;
    //====================================
    tIdxDescr descr = {0, 0, 0, NULL};
    if ((descr.opened_file = fopen(path, "rb")) == NULL ||
            (idx_file_check_and_get_info(&descr)) != IDX_ERR_NONE ||
            descr.data_type != IDX_DT_FLOAT_4B) {
        DEBUG_BREAK;
        ret =  TEST_SUIT_ERROR;
        goto ret_label;
    }

    // Step 3: Check shapes;
    //====================================
    uint32_t shape[MLI_MAX_RANK];
    uint32_t total_pred_elements = mli_hlp_count_elem_num(&pred, 0);
    if (descr.num_elements != total_pred_elements ||
            descr.num_dim != pred.rank) {
        DEBUG_BREAK;
        ret =  TEST_FAILED;
        goto ret_label;
    }

    descr.num_elements = 0;
    idx_file_read_data(&descr, NULL, shape);
    for (uint32_t idx = 0; idx < descr.num_dim; ++idx)
        if (shape[idx] != pred.shape[idx]) {
            DEBUG_BREAK;
            ret = TEST_FAILED;
            goto ret_label;
        }

    // Step 4: Calculate measures;
    //====================================
    pred.rank = 1;
    uint32_t elements_accounted = 0;
    float ref_accum = 0.f;
    float pred_accum = 0.f;
    float noise_accum = 0.f;
    float quant_accum = 0.f;
    float max_abs_err = -1.f;
    const float quant_scale = (float)((int64_t)1l << mli_hlp_tensor_scale_shift(&pred)) / (float)mli_hlp_tensor_scale(&pred, 0);
    const int16_t quant_zero_offset = mli_hlp_tensor_zero_offset(&pred, 0);
    const float quant_max = (1 << (8*pred_elem_size - 1)) - 1.0f;
    const float quant_min = -(1 << (8*pred_elem_size - 1));
    while (elements_accounted  < total_pred_elements) {
        descr.num_elements =
                ((total_pred_elements - elements_accounted ) < data_buf_size) ?
                (total_pred_elements - elements_accounted ):
                data_buf_size;
        pred.shape[0] = descr.num_elements;

        if (idx_file_read_data(&descr, (void *)ref_buf, NULL) != IDX_ERR_NONE ||
                mli_hlp_fx_tensor_to_float(&pred, pred_buf, data_buf_size) != MLI_STATUS_OK) {
            DEBUG_BREAK;
            ret =  TEST_SUIT_ERROR;
            goto ret_label;
        }

        for (uint32_t i = 0; i < descr.num_elements; i++) {
            float ref_quant = ref_buf[i] * quant_scale + quant_zero_offset;
            ref_quant = MAX(quant_min, MIN(quant_max, roundf(ref_quant))) - ref_quant;

            quant_accum += ref_quant * ref_quant;
            ref_accum += ref_buf[i] * ref_buf[i];
            pred_accum += pred_buf[i] * pred_buf[i];
            noise_accum += (ref_buf[i] - pred_buf[i]) * (ref_buf[i] - pred_buf[i]);
            if(fabsf(pred_buf[i] - ref_buf[i]) > max_abs_err) 
                max_abs_err = fabsf(pred_buf[i] - ref_buf[i]);
        }
        elements_accounted += descr.num_elements;
        pred.data += descr.num_elements * pred_elem_size;
    }

    const float eps = 0.000000000000000001f;
    out->max_abs_err = max_abs_err;
    out->quant_err_vec_length = sqrtf(quant_accum)/quant_scale;
    out->pred_vec_length = sqrtf(pred_accum);
    out->ref_vec_length = sqrtf(ref_accum);
    out->noise_vec_length = sqrtf(noise_accum);

    out->noise_to_quant_ratio = (out->noise_vec_length) / (out->quant_err_vec_length + eps);
    out->ref_to_noise_snr = 10.f * log10f((ref_accum + eps) / (noise_accum + eps));

ret_label:
    if (descr.opened_file != NULL)
        fclose(descr.opened_file);
    if (path != NULL)
        free(path);
    if (data != NULL)
        free(data);
    return ret;
}

//================================================================================
// Measure various error metrics between two float vectors
//=================================================================================
test_status measure_err_vfloat(
        const float * ref_vec,
        const float * pred_vec,
        const int len,
        ref_to_pred_output *out) {
    float ref_accum = 0.f;
    float pred_accum = 0.f;
    float noise_accum = 0.f;
    float max_err = -1.f;

    if (len <= 0) {
        DEBUG_BREAK;
        return TEST_FAILED;
    }

    for (int i = 0; i < len; i++) {
        ref_accum += ref_vec[i] * ref_vec[i];
        pred_accum += pred_vec[i] * pred_vec[i];
        noise_accum += (ref_vec[i] - pred_vec[i]) * (ref_vec[i] - pred_vec[i]);
        max_err = MAX(fabsf(ref_vec[i] - pred_vec[i]), max_err);
    }

    const float eps = 0.000000000000000001f;
    out->max_abs_err = max_err;
    out->noise_to_quant_ratio = 1.0f;
    out->quant_err_vec_length = 1.0f;
    out->pred_vec_length = sqrtf(pred_accum);
    out->ref_vec_length = sqrtf(ref_accum);
    out->noise_vec_length = sqrtf(noise_accum);
    out->ref_to_noise_snr = 10.f * log10f((ref_accum + eps) / (noise_accum + eps));
    return TEST_PASSED;
}


test_status fill_asym_tensor_element_params(
        const float * scale_rates,
        const float * zero_points,
        const int num_vals,
        const int scale_int_bits,
        mli_tensor *target_tensor) {
    if (target_tensor->el_type != MLI_EL_ASYM_I8 &&
            target_tensor->el_type != MLI_EL_ASYM_I32) {
        DEBUG_BREAK;
        return TEST_FAILED;
    }
    
    const int8_t scale_fraq_bits = FRAQ_BITS(scale_int_bits, int32_t);
    const uint64_t mult = (uint64_t)1l << FRAQ_BITS(scale_int_bits, int32_t);
    int32_t* scale_dst;
    int16_t* zp_dst;

    if (num_vals > 1) {
        if (target_tensor->el_params.asym.scale.pi32 == NULL ||
                target_tensor->el_params.asym.zero_point.pi16 == NULL) {
            DEBUG_BREAK;
            return TEST_NOT_ENOUGH_MEM;
        }

        scale_dst = target_tensor->el_params.asym.scale.pi32;
        zp_dst = target_tensor->el_params.asym.zero_point.pi16;
    } else {
        scale_dst = &target_tensor->el_params.asym.scale.i32;
        zp_dst = &target_tensor->el_params.asym.zero_point.i16;
    }
    target_tensor->el_params.asym.scale_frac_bits = scale_fraq_bits;

    for (int i = 0; i < num_vals; i++) {
        if (scale_rates[i] <= 0.0f) {
            DEBUG_BREAK;
            return TEST_FAILED;
        }

        const float round_val = 0.5f;

        const int64_t dst_val = (int64_t) (mult * scale_rates[i] + round_val);
        scale_dst[i] = (int32_t) (MIN(MAX(dst_val, INT32_MIN), INT32_MAX));

        const int32_t zero_val = (int32_t)(-zero_points[i] / scale_rates[i] + round_val);
        zp_dst[i] = (int16_t)(MIN(MAX(zero_val , INT16_MIN), INT16_MAX));
    }

    return TEST_PASSED;
}
