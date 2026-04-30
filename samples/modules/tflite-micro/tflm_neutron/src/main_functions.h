/*
 * Copyright 2026 NXP
 * All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef TFLM_CUSTOM_MODEL_MAIN_FUNCTIONS_H_
#define TFLM_CUSTOM_MODEL_MAIN_FUNCTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the TensorFlow Lite Micro model and interpreter.
 *
 * This function sets up the model, creates the interpreter, and allocates
 * the tensor arena. Must be called once before calling loop().
 */
void setup(void);

/**
 * @brief Run one inference cycle.
 *
 * This function performs one complete inference cycle:
 * - Copies image data to input tensor
 * - Invokes the model
 * - Prints the top classification results with labels and confidence scores
 *
 * Should be called repeatedly from the application code.
 */
void loop(void);

#ifdef __cplusplus
}
#endif

#endif /* TFLM_CUSTOM_MODEL_MAIN_FUNCTIONS_H_ */
