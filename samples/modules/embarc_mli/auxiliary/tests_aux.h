/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/

#ifndef _TESTS_AUX_H
#define _TESTS_AUX_H

/**
 * @file Auxiliary functions of MLI Package
 * @brief Various functions for testing and working with MLI library and it's examples
 */

#include <stdint.h>

#include "mli_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @def Macro for calculating number of fractionl bits. Uses container type and number of integer bits*/
#define FRAQ_BITS(int_part, el_type) ((sizeof(el_type)*8)-int_part-1)

#ifndef MAX
/** @def Common Maximum macro function (two values)*/
#define MAX(A,B) (((A) > (B))? (A): (B))
#endif
/** @def Common Minimum macro function (two values)*/
#ifndef MIN
#define MIN(A,B) (((A) > (B))? (B): (A))
#endif

#define CEIL_DIV(num,den) (((num) + (den) - 1)/(den))

//==================================================
//
//      Profiling related functionality
//
//==================================================

/** @def Profiling switcher*/
#define PROFILING_ON

/** @var Cycles counter variable. Will hold number of cycles spent for code surrounded by PROFILE(F) macro*/
extern unsigned cycle_cnt;

/** @def Profiling macro for calculating cyclecount (uses arc specific timer and it's sw interface)*/
#if defined(PROFILING_ON)
#ifdef _ARC
//MWDT toolchain profiling
#include <arc/arc_timer.h>
#define PROFILE(F)  \
    _timer_default_reset();\
    F;\
    cycle_cnt = _timer_default_read();
#else
//GNU toolchain profiling
static inline void test_aux_start_timer_0() {
    _sr(0 , 0x22);
    _sr(0xffffffff, 0x23);
    _sr(3, 0x22);
    _sr(0, 0x21);
}

static inline uint32_t test_aux_read_timer_0() {
    return (_lr(0x21));
}

static inline void test_aux_stop_timer_0() {
    _sr (0 , 0x22);
}

static inline void test_aux_start_timer_1() {
    _sr(0 , 0x101);
    _sr(0xffffffff, 0x102);
    _sr(3, 0x101);
    _sr(0, 0x100);
}

static inline uint32_t test_aux_read_timer_1() {
    return( _lr(0x100) );
}

static inline void test_aux_stop_timer_1() {
    _sr (0 , 0x101);
}

#define PROFILE(F) \
    test_aux_start_timer_0(); \
    F;\
    cycle_cnt = test_aux_read_timer_0();
#endif
#else
#define PROFILE(F) \
    cycle_cnt = 0;\
    F;
#endif

//==================================================
//
//      Data
//
//==================================================

/** @enum Function Return codes  */
typedef enum _test_status {
    TEST_PASSED = 0,        /**< No error occurred */
    TEST_SKIPPED,           /**< Test was skipped */
    TEST_NOT_ENOUGH_MEM,    /**< Not enough memory for test */
    TEST_SUIT_ERROR,        /**< Error insed test suite code */
    TEST_FAILED,            /**< Testing function returns unexpected result */
} test_status;

/** @var Array with discriptive strings according to each error code (test_status_to_str[TEST_PASSED])*/
extern const char *test_status_to_str[];

/** @struct error measurement metrics for two vectors  */
typedef struct ref_to_pred_output_t {
    float max_abs_err;          /**< maximum absolute error  */
    float pred_vec_length;      /**< Length of predicted vector: SQRT(sum_i(pred[i]^2))  */
    float ref_vec_length;       /**< Length of reference vector: SQRT(sum_i(ref[i]^2))  */
    float noise_vec_length;     /**< Length of reference vector: SQRT(sum_i((ref[i]-pred[i])^2))  */
    float quant_err_vec_length; /**< Length of quantized error vector: SQRT(sum_i((ref[i]-Quantized(ref[i]))^2))  */
    float ref_to_noise_snr;     /**< Signal-to-noise ratio 10*log_10((ref_vec_length+eps)/(noise_vec_length+eps))  [dB]*/
    float noise_to_quant_ratio; /**< Noise-to-quantization_err ratio (noise_vec_length)/(quant_err_vec_length+eps)  */
} ref_to_pred_output;

//==================================================
//
//      Functions
//
//==================================================

/**
 * @brief Load several tensors from IDX files
 *
 * @detail Load *paths_num* tensors from external IDX files (test_root/paths[i]) to MLI tensors.
 *         Each tensor must contain sufficient array for storing data of each IDX file accordingly.
 *         Function uses dynamic memory allocation and standart file input/output.
 *         Function function releases all occupied resources before return.
 *
 * @param[in] test_root - root path to all IDX files listed in *paths* array
 * @param[in] paths[] - paths to input IDX files for reading (test_root/paths[i])
 * @param[out] tensors[] - Pointer to output tensors arrayr
 * @param[in] paths_num - Number of idx files for reading (also number of tensors in tensors[] array)
 *
 * @return Operation status code (test_status)
  */
test_status load_tensors_from_idx_files(
        const char * const test_root,
        const char * const paths[],
        mli_tensor * tensors[],
        uint32_t paths_num);

/**
 * @brief Compare data in tensor with external reference data
 *
 * @detail Compare *pred* tensor with reference one stored in external IDX file in terms of various error metrics
 *
 * @param[in] test_root - root path to IDX file Output IDX file path
 * @param[in] ref_vec_path - reference tensor file name
 * @param[in] pred - Input tensor with predicted data
 * @param[out] out - Structures with various error measurements
 *
 * @return Operation status code (test_status)
  */
test_status measure_ref_to_pred(
        const char * const tests_root,
        const char * const ref_vec_file,
        mli_tensor pred,
        ref_to_pred_output *out);

/**
 * @brief Measure various error metrics between two float vectors
 *
 * @detail Compare *ref_vec* against pred_vec in terms of various error metrics
 *
 * @param[in] ref_vec - First array with float values for comparison
 * @param[in] pred_vec - Second array with float values for comparison
 * @param[in] len - Length of both array
 * @param[out] out - Structures with various error measurements
 *
 * @return Operation status code (test_status)
 */
test_status measure_err_vfloat(
        const float * ref_vec,
        const float * pred_vec,
        const int len,
        ref_to_pred_output *out);

/**
 * @brief Fill Asym tesnor parameteres of element base on it's float representations
 *
 * @detail Calculate FX version of scale rates and zero points and store it in target_tensor structure. 
 *         Function might be appied to the following tensor types: MLI_EL_ASYM_I8, MLI_EL_ASYM_I32. 
 *
 * @param[in] scale_rates - Pointer to scale rates in float. 
 * @param[in] zero_points - Pointer to zero point in float. 
 * @param[in] num_vals - Number of values in the input arrays (if > 1, tensor must hold pointers to keep quantized ones)
 * @param[in] scale_int_bits - integer bits of quantized of scale values
 * @param[in/out] target_tensor - Tensor structure to fill. Fields of the structure to be filled beforehand:
 *                                el_type - MLI_EL_ASYM_I8 or MLI_EL_ASYM_I32.
 *                                el_params.asym.dim - quantization axis (negative in case of quantization across whole tensor)
 *                                Additionaly, if el_params.asym.dim >= 0: 
 *                                el_params.asym.zero_point(scale).pi16 - pointers to valid memory,
 *                                                           which can keep num_vals elements of int16_t size
 
 *                                Fields that will be filledby function:
 *                                el_params.asym.scale_frac_bits - number of fractional bits derived from scale_int_bits
 *                                el_params.asym.zero_point - quantized version of zero points (pointer to filled array if num_vals > 1)
 *                                el_params.asym.scale - quantized version of scale rates (pointer to filled array if num_vals > 1)
 *
 * @return Operation status code (test_status)
 */
test_status fill_asym_tensor_element_params(
        const float * scale_rates,
        const float * zero_points,
        const int num_vals,
        const int scale_int_bits,
        mli_tensor *target_tensor);
#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif // _TESTS_AUX_H
