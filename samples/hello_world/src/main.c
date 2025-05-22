/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

int main(void)
{
	printf("Hello World! %s\n", CONFIG_BOARD);
	volatile int n = 10;
    volatile int a[10][10] = {0};
    volatile int b[10][10] = {0};
    volatile int ans[10][10] = {0};

    // Initialize identity matrix for 'a' and 'b'
    for (int i = 0; i < n; i++) {
        a[i][i] = 1;
        b[i][i] = 1;
    }
	

    // Perform matrix multiplication (a * b)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                ans[i][j] += a[i][k] * b[k][j];
            }
        }
    }
	

    // Print the result (which should be an identity matrix)
    printf("Matrix Multiplication Result (should be identity matrix):\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", ans[i][j]);
			
        }
        printf("\n\r");
    }
	


	for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if ((i == j && ans[i][j] != 1) ||  // Diagonal should be exactly 1.0
                (i != j && ans[i][j] != 0)) {  // Off-diagonal should be exactly 0.0
                printf("Wrong output\n\r");
				 return ;
            }
        }
    }
	return 0;
}