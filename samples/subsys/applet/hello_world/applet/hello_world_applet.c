/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hello_world_ext.c
 * @brief Loadable extension for the "hello world" applet sample.
 *
 * This file is compiled into a separate ELF binary that is loaded at runtime
 * by the host application using the Zephyr Applet API. The extension
 * does not need to know anything about LLEXT, memory domains, or userspace;
 * it just exports a single function and uses the standard Zephyr printk API.
 */

#include <zephyr/kernel.h>
#include <zephyr/llext/symbol.h>

/**
 * @brief Extension entry point
 *
 * Called by the applet subsystem after the extension is loaded and its
 * initialisation functions (.init_array) have run.
 *
 * @param arg  Opaque argument forwarded from z_applet_opts.arg; the host
 *             application passes the integer 42 cast to void*.
 */
void applet_main(void *arg)
{
	uintptr_t val = (uintptr_t)arg;

	printk("Hello world from applet 'hello world' (arg=%lu)\n", (unsigned long)val);
}

LL_EXTENSION_SYMBOL(applet_main);
