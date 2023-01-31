/*
 * Copyright (c) 2023 Mindgrove Technologies Pvt Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>

#define MATRIX_SIZE 3

int matrix1[MATRIX_SIZE][MATRIX_SIZE] = {
    {2, 3, 4},
    {5, 6, 7},
    {8, 9, 1}
};

int matrix2[MATRIX_SIZE][MATRIX_SIZE] = {
	{1, 2, 3},
    {3, 4, 6},
    {4, 5, 7}
};

int result[MATRIX_SIZE][MATRIX_SIZE];

void matrix_multiply(void)
{
    int i, j, k;
	int total = 0;
    memset(result, 0, sizeof(result));

    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            for (k = 0; k < MATRIX_SIZE; k++) {
                total += matrix1[i][k] * matrix2[k][j];
            }
			result[i][j] = total;
			total = 0;
        }
    }
}

void main(void)
{
    int i, j;

    matrix_multiply();

    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            printk("%d ", result[i][j]);
        }
        printk("\n");
    }
}
