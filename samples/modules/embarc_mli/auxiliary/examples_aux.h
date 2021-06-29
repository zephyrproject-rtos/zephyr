/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/

#ifndef _EXAMPLES_AUX_H_
#define _EXAMPLES_AUX_H_

/**
 * @file Common example runner functions
 *
 * @brief Set of functions(runners) to organize MLI Library examples in a common way
 */

#include <stdint.h>

#include "mli_api.h"
#include "tests_aux.h"

#ifdef __cplusplus
extern "C" {
#endif

//==================================================
//
//      Data types
//
//==================================================

/** @typedef Common interface for external data preprocessing function */
typedef void (*preproc_func_t)(const void *, mli_tensor *);

/** @typedef Common interface for classifier inference function */
typedef void (*model_inference_t)(const char*);


//==================================================
//
//      Functions
//
//==================================================

/** @brief Run preprocessor and inference function once for provided inputs
 *
 * @detail Function calls sequentially *preprocess* function for input data and *inference* function passing
 *         *inf_param* to it. After inference it calls measure_ref_to_pred function for *ref_out* and *model_output*.
 *
 * @param[in] data_in - Array with input data. Must be applicable for *preprocess* function
 * @param[in] ref_out - Array with model reference output for provided input (data_in).
 *                      Against this data error measurements will be calculated
 * @param[in/out] model_input - Pointer to MLI tensor structure for *inference* input.
 *                              Must be applicable for *preprocess* function and used by *inference* function
 * @param[in/out] model_output - Pointer to MLI tensor structure for *inference* output. Must be used by *inference* function
 * @param[in] preprocess - data preprocessor function. will be called before inference function
 * @param[in] inference - model inference function. Will be called once
 * @param[in] inf_param - Parameter for passing to the inference function
 *
 * @return Operation status code (tests_aux.h)
  */
test_status model_run_single_in(
        const void * data_in,
        const float * ref_out,
        mli_tensor * model_input,
        mli_tensor * model_output,
        preproc_func_t preprocess,
        model_inference_t inference,
        const char * inf_param);


/** @brief Run preprocessor and inference function in cycle for all input dataset with accuracy calculation
 *
 * @detail Function opens input_idx_path dataset and for each entiti calls sequentially *preprocess*
 *         and *inference* function passing *inf_param* to it. It also calculates accuracy according
 *         to *labels_idx_path* file and print intermediate result after passing each 10% of all data
 *
 * @param[in] input_idx_path - path to IDX dataset file for the model
 * @param[in] labels_idx_path - path to IDX reference labels of *input_idx_path* dataset
 * @param[in/out] model_input - Pointer to MLI tensor structure for *inference* input.
 *                              Must be applicable for *preprocess* function and used by *inference* function
 * @param[in/out] model_output - Pointer to MLI tensor structure for *inference* output. Must be used by *inference* function
 * @param[in] preprocess - data preprocessor function. will be called in cycle for each input from dataset.
 * @param[in] inference - model inference function. will be called in cycle for each input from dataset.
 * @param[in] inf_param - Parameter for passing to the inference function
 *
 * @return Operation status code (tests_aux.h)
  */
test_status model_run_acc_on_idx_base(
        const char * input_idx_path,
        const char * labels_idx_path,
        mli_tensor * model_input,
        mli_tensor * model_output,
        preproc_func_t preprocess,
        model_inference_t inference,
        const char * inf_param);


/** @brief Run preprocessor and inference function in cycle for all input dataset. Outputs result to the IDX file
 *
 * @detail Function opens input_idx_path dataset and calls sequentially *preprocess*
 *         and *inference* function for each entiti passing *inf_param* to it.
 *         It fills output_idx_path by model_output values of each inference.
 *
 * @param[in] input_idx_path - path to IDX dataset file for the model
 * @param[in] output_idx_path - path to output IDX file for storing results of each inference
 * @param[in/out] model_input - Pointer to MLI tensor structure for *inference* input.
 *                              Must be applicable for *preprocess* function and used by *inference* function
 * @param[in/out] model_output - Pointer to MLI tensor structure for *inference* output. Must be used by *inference* function
 * @param[in] preprocess - data preprocessor function. will be called in cycle for each input from dataset.
 * @param[in] inference - model inference function. will be called in cycle for each input from dataset.
 * @param[in] inf_param - Parameter for passing to the inference function
 *
 * @return Operation status code (tests_aux.h)
  */
test_status model_run_idx_base_to_idx_out(
        const char * input_idx_path,
        const char * output_idx_path,
        mli_tensor * model_input,
        mli_tensor * model_output,
        preproc_func_t preprocess,
        model_inference_t inference,
        const char * inf_param);


#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif // _EXAMPLES_AUX_H_
