/*
 * Copyright (c) 2024 Linaro LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is the seed point for bindgen.  This determines every header that will be visited for
 * binding generation.  It should include any Zephyr headers we need bindings for.  The driver in
 * build.rs will also select which functions need generation, which will determine the types that
 * are output.
 */

/*
 * This is getting build with KERNEL defined, which causes syscalls to not be implemented.  Work
 * around this by just undefining this symbol.
 */
#undef KERNEL

#ifdef RUST_BINDGEN
/* errno is coming from somewhere in Zephyr's build.  Add the symbol when running bindgen so that it
 * is defined here.
 */
extern int errno;
#endif

/* First, make sure we have all of our config settings. */
#include <zephyr/autoconf.h>

/*
 * Gcc defines __SOFTFP__ when the target doesn't use hardware
 * floating point.  For clang, this isn't getting defined, which stops
 * CMSIS headers.  We can work around this by just defining this using
 * the same logic we use for hard vs soft float.
 */
#ifdef CONFIG_CPU_CORTEX_M
  #if !(defined(CONFIG_FP_HARDAI) || defined(FORCE_FP_HARDABI))
    #ifndef __SOFTFP__
      #define __SOFTFP__
    #endif
  #endif
#endif

#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/drivers/gpio.h>

/*
 * bindgen will only output #defined constants that resolve to simple numbers.  These are some
 * symbols that we want exported that, at least in some situations, are more complex, usually with a
 * type cast.
 *
 * We'll use the prefix "ZR_" to avoid conflicts with other symbols.
 */
const uintptr_t ZR_STACK_ALIGN = Z_KERNEL_STACK_OBJ_ALIGN;
const uintptr_t ZR_STACK_RESERVED = K_KERNEL_STACK_RESERVED;
