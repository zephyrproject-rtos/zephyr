/*
 * Copyright (c) 2023 Mindgrove Technologies Pvt Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/timing/timing.h>

#define MATRIX_SIZE 4

int matrix1[MATRIX_SIZE][MATRIX_SIZE] = {
    {2, 3, 4, 1},
    {5, 6, 7, 0},
    {8, 9, 1, 3},
    {1, 2, 3, 4}
};

int matrix2[MATRIX_SIZE][MATRIX_SIZE] = {
  {1, 2, 3, 4},
    {3, 4, 6, 5},
    {4, 5, 7, 9},
    {6, 8, 2, 5}
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

void mcycle_init(){
  uint64_t mcycle = 0x00000000;
  __asm__ volatile("csrw mcycle,%0":"=r"(mcycle));
}

inline uint64_t get_mcycle_start() {
  uint64_t mcycle = 0;
  __asm__ volatile("csrr %0,mcycle":"=r"(mcycle));
  return mcycle;
}

inline uint64_t get_mcycle_stop(){
  uint64_t mcycle;
  
  __asm__ volatile("csrr %0,mcycle":"=r"(mcycle));
  return mcycle;
}

void main(void)
{
    int i, j;
    timing_t start, end;
    uint64_t total_cycles;
    float total_ns;
    uint64_t start_mcycle, stop_mcycle;
    float sum_time=0;
    int sum=0;
    int it = 10000;

    
    printk("\n ...Hello World again...");
    
    printk("\n INPUT MATRIX A: \n");

    for(i=0; i<MATRIX_SIZE; i++){
        for(j=0; j<MATRIX_SIZE; j++){
            printk("%d ", matrix1[i][j]);
        }
	printk("\n");
    }

    printk("\n INPUT MATRIX B: \n");

    for(i=0; i<MATRIX_SIZE; i++){
        for(j=0; j<MATRIX_SIZE; j++){
            printk("%d ", matrix2[i][j]);
        }
	printk("\n");
    }
    
    printk("\n Lets check the output of the 4x4 Matrix Multiplication...");
   // printk("\n Generating Matrices..");
   // for(int i=0; i<MATRIX_SIZE; i++){
   //     for(int j=0; j<MATRIX_SIZE; j++){
   //        matrix1[i][j] = 1;
   //     }
   // }

   // for(int i=0; i<MATRIX_SIZE; i++){
   //     for(int j=0; j<MATRIX_SIZE; j++){
   //         matrix2[i][j] = 1;
   //     }
   // }

    printk("\n GOING TO CALL MATRIX-MULTIPLY");
    mcycle_init();
    
    for(i=0; i<it; i++){
         start_mcycle = get_mcycle_start();
         matrix_multiply();
         stop_mcycle = get_mcycle_stop();
	 if (i == 0)
	   continue;
	 
         total_cycles = stop_mcycle - start_mcycle;//timing_cycles_get(&start, &end);
         total_ns = timing_cycles_to_ns(total_cycles);
	 sum += total_cycles;
	 //printk("\n i=%d \n SUM = %d",i,sum);
	 sum_time += total_ns;
	 //printk("\nSUM_TIME: %f", sum_time);
    }
    printk("\n OUTPUT MATRIX: \n");

    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            printk("%d ", result[i][j]);
        }
        printk("\n");
    }

    //printk("");
    //printk("");
    //printk("\nSTART TIME: %u", start_mcycle);
    //    printk("\nEND TIME: %u", stop_mcycle);
    printk("\n total iteration: %d",it);
    printk("\n Average Number of Cycles: %d",sum/it);
    printk("\n Total Number of Cycles: %d",total_cycles);
    printk("\n TIME TAKEN IN NS: %f ns", sum_time/it);
    printk("\n OVERALL TIME TAKEN: %.15f ms \n", (sum_time/it)/1000000);
}
