/*
 * Copyright (c) 2019 Kevin Townsend
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zsl/zsl.h>
#include <zsl/matrices.h>

void test_mtx_mult_demo(void)
{
	int rc;

	ZSL_MATRIX_DEF(ma, 4, 3);       /* 4x3 matrix */
	ZSL_MATRIX_DEF(mb, 3, 4);       /* 3x4 matrix */
	ZSL_MATRIX_DEF(mc, 4, 4);       /* Resulting 4x4 matrix. */

	zsl_real_t ma_data[12] = {
		1.0, 2.0, 3.0,
		4.0, 5.0, 6.0,
		7.0, 8.0, 9.0,
		1.0, 1.0, 1.0
	};

	zsl_real_t mb_data[12] = {
		1.0, 2.0, 3.0, 4.0,
		5.0, 6.0, 7.0, 8.0,
		1.0, 1.0, 1.0, 1.0
	};

	/* Assign data to input matrices. */
	ma.data = ma_data;
	mb.data = mb_data;

	/* Initialise output matrix. */
	zsl_mtx_init(&mc, NULL);

	/* Multiply ma x mb. */
	rc = zsl_mtx_mult(&ma, &mb, &mc);
	if (rc) {
		printf("Error performing matrix multuplication: %d\n", rc);
		return;
	}

	printf("mtx multiply output (4x3 * 3x4 = 4x4):\n\n");
	zsl_mtx_print(&mc);
}

void main(void)
{
	printf("zscilib matrix mult demo\n\n");

	test_mtx_mult_demo();

	while (1) {
		k_sleep(K_FOREVER);
	}
}
