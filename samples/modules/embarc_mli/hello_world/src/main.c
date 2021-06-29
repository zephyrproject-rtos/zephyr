/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mli_api.h"
#include <assert.h>
#include <zephyr.h>
#include <stdio.h>

int main(void)
{
	printf("************************************\n");
	int8_t data_in[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	mli_tensor in = { 0 };

	in.el_type = MLI_EL_FX_8;
	in.rank = 1;
	in.shape[0] = 8;
	in.mem_stride[0] = 1;
	in.capacity = sizeof(data_in);
	in.data = data_in;

	int8_t data_out[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	mli_tensor out = { 0 };

	out.el_type = MLI_EL_FX_8;
	out.rank = 1;
	out.shape[0] = 8;
	out.mem_stride[0] = 1;
	out.capacity = sizeof(data_out);
	out.data = data_out;

	mli_status status;

	status = mli_krn_eltwise_add_fx8(&in, &in, &out);
	assert(status == MLI_STATUS_OK);
	for (int i = 0; i < ARRAY_SIZE(data_out); i++) {
		printf("%d ", ((int8_t *)(out.data))[i]);
	}
	printf("\n");

	status = mli_krn_eltwise_sub_fx8(&in, &in, &out);
	assert(status == MLI_STATUS_OK);
	for (int i = 0; i < ARRAY_SIZE(data_out); i++) {
		printf("%d ", ((int8_t *)(out.data))[i]);
	}
	printf("\n");
	printf("************************************\n");

	printf("Hello World! %s\n", CONFIG_BOARD);
	return 0;
}
