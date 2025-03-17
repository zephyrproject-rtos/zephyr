/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_intsup.h>
#include <xnnpack.h> // Include XNNPack headers
#include <zephyr/sys/reboot.h>
volatile int wait = 0;

unsigned long cycle()
{
	unsigned long cc;
	__asm__ volatile("rdcycle  %0" : "=r"(cc));
	return cc;
}

int main(void)
{
	printf("Hello World! Running XNNPACK FP32 Test\n");

	// Test malloc
	void *test_alloc = malloc(1024);
	if (!test_alloc)
	{
		printf("Test malloc failed!\n");
		return -1;
	}
	printf("Test malloc succeeded!\n");
	free(test_alloc);

	// Initialize XNNPACK
	int status = xnn_initialize(NULL);
	if (status != xnn_status_success)
	{
		printf("Failed to initialize XNNPack, status code: %d\n", status);
		return -1;
	}
	printf("XNNPACK initialized successfully!\n");

	const size_t batch_size = 1;
	const size_t input_channels = 1024;
	const size_t output_channels = 64;

	float *input_data = (float *)malloc(input_channels * sizeof(float));
	float *weights = (float *)malloc(input_channels * output_channels * sizeof(float));
	float *bias = (float *)malloc(output_channels * sizeof(float));
	float *output_data = (float *)malloc(output_channels * sizeof(float));

	printf("Test shapes: %zu, %zu\n", input_channels, output_channels);

	printf("Preparing input data and weights\n");
	// Initialize input data
	for (size_t i = 0; i < input_channels; i++)
	{
		input_data[i] = (float)i;
	}
	// Initialize weights
	for (size_t i = 0; i < input_channels * output_channels; i++)
	{
		weights[i] = (float)i * i;
	}

	printf("Creating operators\n");
	// Create the Fully Connected operator
	xnn_operator_t fc_op = NULL;
	status = xnn_create_fully_connected_nc_f32(input_channels,	// Input size per batch
											   output_channels, // Output size per batch
											   input_channels,	// Input stride
											   output_channels, // Output stride
											   weights,			// Weights matrix
											   bias,			// Bias vector
											   -INFINITY,		// Min activation
											   INFINITY,		// Max activation
											   0,				// Flags
											   NULL,			// Code cache
											   NULL,			// Weights cache
											   &fc_op);

	if (status != xnn_status_success)
	{
		printf("Failed to create Fully Connected operator, status code: %d\n", status);
		return -1;
	}

	// Reshape the operator
	status = xnn_reshape_fully_connected_nc_f32(fc_op, 1, NULL);
	if (status != xnn_status_success)
	{
		printf("Failed to reshape Fully Connected operator, status code: %d\n", status);
		xnn_delete_operator(fc_op);
		return -1;
	}

	// Setup the operator
	status = xnn_setup_fully_connected_nc_f32(fc_op, input_data, output_data);
	if (status != xnn_status_success)
	{
		printf("Failed to setup Fully Connected operator, status code: %d\n", status);
		xnn_delete_operator(fc_op);
		return -1;
	}

	printf("Shape of input and output data: %d, %d\n", input_channels, output_channels);

	unsigned long clock_start = cycle();

	// Run the operator
	status = xnn_run_operator(fc_op, NULL);
	if (status != xnn_status_success)
	{
		printf("Failed to run Fully Connected operator, status code: %d\n", status);
		xnn_delete_operator(fc_op);
		return -1;
	}

	unsigned long clock_end = cycle();
	printf("Clocks taken: %ld\n", (clock_end - clock_start));

	// Print results
	printf("Fully Connected Output:\n");
	for (size_t i = 0; i < output_channels; i++)
	{
		printf("%f ", (double)output_data[i]); // Explicitly cast to avoid warnings
	}
	printf("\n");

	// Cleanup
	xnn_delete_operator(fc_op);

	sys_reboot(SYS_REBOOT_COLD);
	return 0;
}