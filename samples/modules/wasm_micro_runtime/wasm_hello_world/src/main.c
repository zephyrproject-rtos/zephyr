/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "bh_platform.h"
#include "wasm_export.h"
#include "test_wasm.h"

#define GLOBAL_HEAP_BUF_SIZE 131072
#define APP_STACK_SIZE 8192
#define APP_HEAP_SIZE 8192

#ifdef CONFIG_NO_OPTIMIZATIONS
#define MAIN_THREAD_STACK_SIZE 8192
#else
#define MAIN_THREAD_STACK_SIZE 4096
#endif

#define MAIN_THREAD_PRIORITY 5

static int app_argc;
static char **app_argv;

static void*
app_instance_main(wasm_module_inst_t module_inst)
{
	const char *exception;

	wasm_application_execute_main(module_inst, app_argc, app_argv);
	exception = wasm_runtime_get_exception(module_inst);
	if (exception != NULL)
		printf("%s\n", exception);
	return NULL;
}

static char global_heap_buf[GLOBAL_HEAP_BUF_SIZE] = { 0 };

void
iwasm_main(void *arg1, void *arg2, void *arg3)
{
	int start = k_uptime_get_32(), end;
	uint8 *wasm_file_buf = NULL;
	uint32 wasm_file_size;
	wasm_module_t wasm_module = NULL;
	wasm_module_inst_t wasm_module_inst = NULL;
	RuntimeInitArgs init_args;
	char error_buf[128];
	int log_verbose_level = 2;

	(void) arg1;
	(void) arg2;
	(void) arg3;

	memset(&init_args, 0, sizeof(RuntimeInitArgs));

	init_args.mem_alloc_type = Alloc_With_Pool;
	init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
	init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

	/* initialize runtime environment */
	if (!wasm_runtime_full_init(&init_args)) {
		printf("Init runtime environment failed.\n");
		return;
	}

	bh_log_set_verbose_level(log_verbose_level);

	/* load WASM byte buffer from byte buffer of include file */
	wasm_file_buf = (uint8 *)wasm_test_file;
	wasm_file_size = sizeof(wasm_test_file);

	/* load WASM module */
	wasm_module =
			wasm_runtime_load(wasm_file_buf,
							  wasm_file_size,
							  error_buf,
							  sizeof(error_buf));
	if (!wasm_module) {
		printf("%s\n", error_buf);
		goto fail1;
	}

	/* instantiate the module */
	wasm_module_inst =
		wasm_runtime_instantiate(wasm_module,
							APP_STACK_SIZE,
							APP_HEAP_SIZE,
							error_buf,
							sizeof(error_buf));
	if (!wasm_module_inst) {
		printf("%s\n", error_buf);
		goto fail3;
	}

	/* invoke the main function */
	app_instance_main(wasm_module_inst);

	/* destroy the module instance */
	wasm_runtime_deinstantiate(wasm_module_inst);

fail3:
	/* unload the module */
	wasm_runtime_unload(wasm_module);

fail1:
	/* destroy runtime environment */
	wasm_runtime_destroy();
	end = k_uptime_get_32();
	printf("elpase: %d\n", (end - start));
}

K_THREAD_STACK_DEFINE(iwasm_main_thread_stack, MAIN_THREAD_STACK_SIZE);
static struct k_thread iwasm_main_thread;

static bool iwasm_init(void)
{
	k_tid_t tid = k_thread_create(&iwasm_main_thread,
						iwasm_main_thread_stack,
						MAIN_THREAD_STACK_SIZE,
						iwasm_main,
						NULL, NULL, NULL,
						MAIN_THREAD_PRIORITY,
						0, K_NO_WAIT);
	return tid ? true : false;
}

int main(void)
{
	bool success = iwasm_init();

	return success ? 0 : -1;
}
