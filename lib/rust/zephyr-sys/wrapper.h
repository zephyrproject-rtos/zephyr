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

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/random/random.h>