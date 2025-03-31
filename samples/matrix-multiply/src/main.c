/*
 * Copyright (c) 2023 Mindgrove Technologies Pvt Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/timing/timing.h>


char *mglogo_v2 = "\n\
                          **.**              **..\n\
                       ***.***.***        *.*.***.***\n\
                   *.*..**     .*..**   .*.*.     .*..*.*\n\
                  ***.*           .*..***.*          *.*.**.\n\
                  .*                 *.*.*.*             ***.***\n\
         **       .*                     .*..**              .*..**\n\
      ***...*.*   .*        **.**           .*.                 ***\n\
  *.*..*.  ***.**..*          *.**.*                            ***\n\
**.*.*         .******           *..**.*              *..       ***\n\
*.                 .**.***          .*.***.*      *.***.*       ***\n\
*.                     .*..*.       .**  *.***.*.****           **.\n\
*.        ****            *.*       .**     *.**.*           .*.**..**\n\
*.          *..**                   .**       **          .*..**   **..**\n\
*.             **..**               ***       **        ...**         **..*\n\
*.                .....**       *.**.*        **         *               **\n\
*.                ..  *.*..* *.**.*           **                         **\n\
*.        ***     ..     **.*.**              .***            *.*.       **\n\
*.         **..*  ..                           *.***.     *.***.*        **\n\
*..           **....                ***           *.***.***.*            **\n\
*.*.*.            *...**        *..**.**.*        *.***.*            *.*.**\n\
   **..**            *....*  *.**.*    *.***.* *.***.*            ***.*.*\n\
      **..*             *.***.**           *.***.*              ...*.\n\
        .*.           *....*                                    ***\n\
        .*.        *...**                                       ***\n\
        .*.        **            .**.*              ***.*       ***\n\
        .*.                  ***.***             .*..*.         ***\n\
        .*.              *.*..***            *.*.***           ****\n\
        .*.           **..*.* .*.*.*      .*..*..*..**      **..**\n\
        .*.       *.*.**.        .*.*.*.*.***      .*..*..*..**\n\
        .*.   *.*.**.               .*.*.*            ******\n\
        .*..*..*.*\n\
        .*.***\n\
        .**\n\
";

char *bootlogo = "\n\
                                       ./((*\n\
                                   ,(((((,\n\
                               *((((((,\n\
                          ./(((((((,\n\
                      ./((((((((*\n\
                   *(((((((((/\n\
               .(((((((((((,\n\
            ,((((((((((((/\n\
          ((((((((((((((/\n\
         .((((((((((((((/\n\
             *(((((((((((.\n\
                  /(((((((.\n\
                ,.     *(((/\n\
                    *((,     ,/.\n\
                      ((((((/.\n\
                       ((((((((((/\n\
                        (((((((((((((/\n\
                        ((((((((((((((.\n\
                       ((((((((((((/\n\
                     *((((((((((*\n\
                   ((((((((((.\n\
                /((((((((*\n\
             *(((((((,\n\
          *((((((.\n\
      .(((((.\n\
  ./(((*\n\
  .\n";

char *bootstring ="\n\
                    SHAKTI PROCESSORS\n";


char *mglogo_v2 = "\n\
                          **.**              **..\n\
                       ***.***.***        *.*.***.***\n\
                   *.*..**     .*..**   .*.*.     .*..*.*\n\
                  ***.*           .*..***.*          *.*.**.\n\
                  .*                 *.*.*.*             ***.***\n\
         **       .*                     .*..**              .*..**\n\
      ***...*.*   .*        **.**           .*.                 ***\n\
  *.*..*.  ***.**..*          *.**.*                            ***\n\
**.*.*         .******           *..**.*              *..       ***\n\
*.                 .**.***          .*.***.*      *.***.*       ***\n\
*.                     .*..*.       .**  *.***.*.****           **.\n\
*.        ****            *.*       .**     *.**.*           .*.**..**\n\
*.          *..**                   .**       **          .*..**   **..**\n\
*.             **..**               ***       **        ...**         **..*\n\
*.                .....**       *.**.*        **         *               **\n\
*.                ..  *.*..* *.**.*           **                         **\n\
*.        ***     ..     **.*.**              .***            *.*.       **\n\
*.         **..*  ..                           *.***.     *.***.*        **\n\
*..           **....                ***           *.***.***.*            **\n\
*.*.*.            *...**        *..**.**.*        *.***.*            *.*.**\n\
   **..**            *....*  *.**.*    *.***.* *.***.*            ***.*.*\n\
      **..*             *.***.**           *.***.*              ...*.\n\
        .*.           *....*                                    ***\n\
        .*.        *...**                                       ***\n\
        .*.        **            .**.*              ***.*       ***\n\
        .*.                  ***.***             .*..*.         ***\n\
        .*.              *.*..***            *.*.***           ****\n\
        .*.           **..*.* .*.*.*      .*..*..*..**      **..**\n\
        .*.       *.*.**.        .*.*.*.*.***      .*..*..*..**\n\
        .*.   *.*.**.               .*.*.*            ******\n\
        .*..*..*.*\n\
        .*.***\n\
        .**\n\
";

char *bootlogo = "\n\
                                       ./((*\n\
                                   ,(((((,\n\
                               *((((((,\n\
                          ./(((((((,\n\
                      ./((((((((*\n\
                   *(((((((((/\n\
               .(((((((((((,\n\
            ,((((((((((((/\n\
          ((((((((((((((/\n\
         .((((((((((((((/\n\
             *(((((((((((.\n\
                  /(((((((.\n\
                ,.     *(((/\n\
                    *((,     ,/.\n\
                      ((((((/.\n\
                       ((((((((((/\n\
                        (((((((((((((/\n\
                        ((((((((((((((.\n\
                       ((((((((((((/\n\
                     *((((((((((*\n\
                   ((((((((((.\n\
                /((((((((*\n\
             *(((((((,\n\
          *((((((.\n\
      .(((((.\n\
  ./(((*\n\
  .\n";

char *bootstring ="\n\
                    SHAKTI PROCESSORS\n";

#define MATRIX_SIZE 4

int matrix1[MATRIX_SIZE][MATRIX_SIZE] = {
    {2, 3, 4, 1},
    {5, 6, 7, 0},
    {8, 9, 1, 3},
    {1, 2, 3, 4}
};

// float matrix1[MATRIX_SIZE][MATRIX_SIZE] = {
// {1.39, 4.39, 6.49, 8.18},
// {7.68, 5.86, 2.42, 0.37},
// {3.96, 7.04, 3.54, 6.20},
// {8.73, 8.96, 7.92, 8.59},
// };

// float matrix1[MATRIX_SIZE][MATRIX_SIZE] = {
// {1.39, 4.39, 6.49, 8.18},
// {7.68, 5.86, 2.42, 0.37},
// {3.96, 7.04, 3.54, 6.20},
// {8.73, 8.96, 7.92, 8.59},
// };

int matrix2[MATRIX_SIZE][MATRIX_SIZE] = {
    {1, 2, 3, 4},
    {1, 2, 3, 4},
    {3, 4, 6, 5},
    {4, 5, 7, 9},
    {6, 8, 2, 5}
};

// float matrix2[MATRIX_SIZE][MATRIX_SIZE] = {
//     {-0.48, -0.35, -0.10, 0.55},
//     {0.40, 0.46, 0.24, -0.58},
//     {0.62, 0.50, -0.28, -0.41},
//     {-0.51, -0.58, 0.10, 0.54}
// };

// float matrix2[MATRIX_SIZE][MATRIX_SIZE] = {
//     {-0.48, -0.35, -0.10, 0.55},
//     {0.40, 0.46, 0.24, -0.58},
//     {0.62, 0.50, -0.28, -0.41},
//     {-0.51, -0.58, 0.10, 0.54}
// };

int result[MATRIX_SIZE][MATRIX_SIZE];

// double result[MATRIX_SIZE][MATRIX_SIZE];

// double result[MATRIX_SIZE][MATRIX_SIZE];

void matrix_multiply(void)
{
    int i, j, k;
	int total = 0;
    
    // double total = 0.0;
    
    // double total = 0.0;
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

    
    // printk("\n ...Hello World again...");

    printk("\n\t\t\t      MINDGROVE SILICON™\n %s\n",mglogo_v2);
	printk("          Copyright © 2024 Mindgrove Technologies Pvt Ltd.\n\n");
	printk("\t\t\t    POWERED BY\n");
	printk("%s\n\n\t\t  SHAKTI C-Class PROCESSOR \n\n",bootlogo);
	printk("\n","Booting... \n");

	printk("\n \nMatrix Multiplication \n");
    // printk("\n ...Hello World again...");

    printk("\n\t\t\t      MINDGROVE SILICON™\n %s\n",mglogo_v2);
	printk("          Copyright © 2024 Mindgrove Technologies Pvt Ltd.\n\n");
	printk("\t\t\t    POWERED BY\n");
	printk("%s\n\n\t\t  SHAKTI C-Class PROCESSOR \n\n",bootlogo);
	printk("\n","Booting... \n");

	printk("\n \nMatrix Multiplication \n");
    
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
    
    // printk("\n Lets check the output of the 4x4 Matrix Multiplication...");
    // printk("\n Lets check the output of the 4x4 Matrix Multiplication...");
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

    // printk("\n GOING TO CALL MATRIX-MULTIPLY");
    // printk("\n GOING TO CALL MATRIX-MULTIPLY");
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
    printk("\n Total iterations: %d",it);
    printk("\n Average number of Cycles: %d",sum/it);
    printk("\n Total number of Cycles: %d",total_cycles);
    printk("\n Time taken in Nano-Seconds: %f ns", sum_time/it);
    printk("\n Overall time taken in Milli-Seconds: %.15f ms \n", (sum_time/it)/1000000);
    printk("\n Total iterations: %d",it);
    printk("\n Average number of Cycles: %d",sum/it);
    printk("\n Total number of Cycles: %d",total_cycles);
    printk("\n Time taken in Nano-Seconds: %f ns", sum_time/it);
    printk("\n Overall time taken in Milli-Seconds: %.15f ms \n", (sum_time/it)/1000000);
}