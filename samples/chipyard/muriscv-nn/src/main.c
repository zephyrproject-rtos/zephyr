/*
 * Copyright (c) 2025 Dima Nikiforov <vnikiforov@berkeley.edu>
 * SPDX-License-Identifier: Apache-2.0
 *
 * A simple inference example using muRISCV-NN fully connected function.
 * This example sets up dummy parameters and calls the inference.
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/sys/reboot.h>
#include <muriscv_nn_functions.h>

/* Dummy dimensions and parameters for the fully connected layer.
 * Adjust these values and array sizes according to your model.
 */
#define INPUT_BATCHES    1
#define INPUT_W          10
#define INPUT_H          1
#define INPUT_CHANNELS   10
#define FILTER_DEPTH     10   /* accumulation depth */
#define OUTPUT_CHANNELS  5

#define MSTATUS_VS          0x00000600
#define MSTATUS_FS          0x00006000
#define MSTATUS_XS          0x00018000

static inline void enable_vector_operations(void) {
    unsigned long mstatus;

    /* Read current mstatus */
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));

    /* Set VS, FS, and XS fields to Dirty */
    mstatus |= MSTATUS_VS | MSTATUS_FS | MSTATUS_XS;

    /* Write back updated mstatus */
    __asm__ volatile("csrw mstatus, %0" :: "r"(mstatus));
}

int main(void)
{
    printf("Starting simple muRISCV-NN inference example\n");

	enable_vector_operations();

    /* Allocate a context buffer.
     * In real usage, get the required size from:
     *   muriscv_nn_fully_connected_s8_get_buffer_size(&filter_dims);
     * Here we use a dummy size.
     */
    muriscv_nn_context ctx;
    const int32_t dummy_buf_size = 128;
    ctx.buf = malloc(dummy_buf_size);
    ctx.size = dummy_buf_size;

    /* Set up fully connected parameters.
     * These values are dummy and should be replaced by your model’s parameters.
     */
    muriscv_nn_fc_params fc_params;
    fc_params.input_offset  = 0;
    fc_params.filter_offset = 0;
    fc_params.output_offset = 0;
    fc_params.activation.min = -128;
    fc_params.activation.max = 127;

    muriscv_nn_per_tensor_quant_params quant_params;
    quant_params.multiplier = 1;
    quant_params.shift      = 0;

    /* Set up tensor dimensions for input, filter and output. */
    muriscv_nn_dims input_dims;
    input_dims.n = INPUT_BATCHES;
    input_dims.w = INPUT_W;
    input_dims.h = INPUT_H;
    input_dims.c = INPUT_CHANNELS;

    muriscv_nn_dims filter_dims;
    filter_dims.n = FILTER_DEPTH;      /* accumulation depth */
    filter_dims.c = OUTPUT_CHANNELS;   /* number of output channels */

    /* Bias dimensions can be left uninitialized or set as needed. */
    muriscv_nn_dims bias_dims = {0};

    muriscv_nn_dims output_dims;
    output_dims.n = INPUT_BATCHES;
    output_dims.c = OUTPUT_CHANNELS;

    /* Allocate dummy arrays.
     * Replace these with actual data for a real inference.
     */
    q7_t input_data[INPUT_W * INPUT_H * INPUT_CHANNELS] = {0};
    q7_t kernel_data[FILTER_DEPTH * OUTPUT_CHANNELS] = {0};
    q31_t bias_data[OUTPUT_CHANNELS] = {0};
    q7_t output[OUTPUT_CHANNELS] = {0};

    /* Run the fully connected inference */
    muriscv_nn_status status = muriscv_nn_fully_connected_s8(&ctx,
                                                             &fc_params,
                                                             &quant_params,
                                                             &input_dims,
                                                             input_data,
                                                             &filter_dims,
                                                             kernel_data,
                                                             &bias_dims,
                                                             bias_data,
                                                             &output_dims,
                                                             output);

    /* Free the temporary context buffer */
    free(ctx.buf);

    if (status == MURISCV_NN_SUCCESS) {
        printf("Inference successful\n");
    } else {
        printf("Inference failed with status: %d\n", status);
    }

    /* Optionally print the output values */
    printf("Output values: ");
    for (int i = 0; i < output_dims.c; i++) {
        printf("%d ", output[i]);
    }
    printf("\n");

    /* Reboot the system (as in your test template) */
    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}