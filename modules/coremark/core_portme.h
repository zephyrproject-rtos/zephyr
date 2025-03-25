/*
 * Copyright (c) 2025 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CORE_PORTME_H
#define CORE_PORTME_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdlib.h>

/* Basic coremark configuration */
#ifndef HAS_FLOAT
  #define HAS_FLOAT 1
#endif

#ifndef HAS_TIME_H
  #define HAS_TIME_H 0
#endif

#ifndef USE_CLOCK
  #define USE_CLOCK 0
#endif

#ifndef HAS_STDIO
  #define HAS_STDIO 0
#endif

#ifndef HAS_PRINTF
  #define HAS_PRINTF 0
#endif

#ifndef MAIN_HAS_NOARGC
  #define MAIN_HAS_NOARGC 1
#endif

#ifndef MAIN_HAS_NORETURN
  #define MAIN_HAS_NORETURN 1
#endif

#ifndef SEED_METHOD
  #define SEED_METHOD SEED_VOLATILE
#endif

/* Compiler information setup */
#ifndef COMPILER_VERSION
  #ifdef __GNUC__
    #define COMPILER_VERSION "GCC"__VERSION__
  #else
    #define COMPILER_VERSION "Please put compiler version here (e.g. gcc 4.1)"
  #endif
#endif

#ifndef COMPILER_FLAGS
  #define COMPILER_FLAGS CONFIG_COMPILER_OPT
#endif

/* Definition of the method to get a block of memory */
#ifndef MEM_METHOD
  #ifdef CONFIG_COREMARK_MEMORY_METHOD_STACK
    #define MEM_METHOD    MEM_STACK
    #define MEM_LOCATION  "STACK"
  #elif defined(CONFIG_COREMARK_MEMORY_METHOD_STATIC)
    #define MEM_METHOD    MEM_STATIC
    #define MEM_LOCATION  "STATIC"
  #elif defined(CONFIG_COREMARK_MEMORY_METHOD_MALLOC)
    #define MEM_METHOD    MEM_MALLOC
    #define MEM_LOCATION  "MALLOC"
  #endif
#endif

#ifndef MULTITHREAD
  #define MULTITHREAD CONFIG_COREMARK_THREADS_NUMBER
#endif

#if (MULTITHREAD > 1)
  #define PARALLEL_METHOD "PThreads"
#endif

/* Depending on the benchmark run type different data size is required.
 * Default values for TOTAL_DATA_SIZE is predefined in coremark.h
 * Redefine it here and get value according to coremark configuraion.
 */
#undef TOTAL_DATA_SIZE
#define TOTAL_DATA_SIZE  CONFIG_COREMARK_DATA_SIZE

/* Coremark's entry point by default called 'main'.
 * As CoreMark sources are readonly we need to solve this conflict in other way.
 * In order to use it we need somehow to rename coremark main functon.
 */
#define main       coremark_run

/* crc16 function is present also in zephyr and coremark.
 * To avoid multiple definition error without changing benchmark's code,
 * coremark crc16 instance is renamed.
 */
#define crc16      coremark_crc16

/* This function will be used to output benchmark results */
#define ee_printf  printk

/* Functions used by coremark for MEM_METHOD MALLOC case */
#define portable_malloc  k_malloc
#define portable_free    k_free

/* Coremark specific data types definition
 * Note, that ee_ptr_int is required to be the data type used to hold pointers
 */
typedef signed short   ee_s16;
typedef unsigned short ee_u16;
typedef signed int     ee_s32;
typedef double         ee_f32;
typedef unsigned char  ee_u8;
typedef unsigned int   ee_u32;
typedef ee_u32         ee_ptr_int;
typedef size_t         ee_size_t;
typedef uint32_t       CORE_TICKS;

/* align_mem macro is used to align an offset to point to a 32b value. It is
 * used in the Matrix algorithm to initialize the input memory blocks.
 */
#define align_mem(x) (void *)(4 + (((ee_ptr_int)(x) - 1) & ~3))

typedef struct CORE_PORTABLE_S {
	ee_u8 portable_id;
} core_portable;

extern ee_u32 default_num_contexts;

void portable_init(core_portable *p, int *argc, char *argv[]);
void portable_fini(core_portable *p);

#endif /* CORE_PORTME_H */
