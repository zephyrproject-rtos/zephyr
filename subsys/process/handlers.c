/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/process/process.h>
#include <zephyr/internal/syscall_handler.h>

#define VRFY_STRING_MAXLEN 256
#define VRFY_ARGS_MAX_ARGC 64

static inline int vrfy_string(const char *str)
{
	size_t len;
	int err;

	len = k_usermode_string_nlen(str, VRFY_STRING_MAXLEN, &err);

	if (err) {
		return -EINVAL;
	}

	if (len == VRFY_STRING_MAXLEN) {
		return -EINVAL;
	}

	return K_SYSCALL_MEMORY_READ(str, len + 1);
}

static inline struct k_process *z_vrfy_k_process_get(const char *name)
{
	K_OOPS(vrfy_string(name));
	return z_impl_k_process_get(name);
}
#include <zephyr/syscalls/k_process_get_mrsh.c>

static inline int vrfy_argc(size_t argc)
{
	return argc > VRFY_ARGS_MAX_ARGC ? -EINVAL : 0;
}

static inline int vrfy_argv_array(size_t argc, const char **argv)
{
	return K_SYSCALL_MEMORY_READ(argv, sizeof(const char *) * argc);
}

static inline int vrfy_argv_strings(size_t argc, const char **argv)
{
	for (size_t i = 0; i < argc; i++) {
		if (vrfy_string(argv[i])) {
			return -EINVAL;
		}
	}

	return 0;
}

static inline int vrfy_pipe(struct k_pipe *pipe)
{
	if (pipe == NULL) {
		return 0;
	}

	return K_SYSCALL_OBJ(pipe, K_OBJ_PIPE);
}

static inline int vrfy_poll_signal(struct k_poll_signal *signal)
{
	if (signal == NULL) {
		return 0;
	}

	return K_SYSCALL_OBJ(signal, K_OBJ_POLL_SIGNAL);
}

static inline int z_vrfy_k_process_start(struct k_process *process,
					 size_t argc,
					 const char **argv,
					 struct k_pipe *input,
					 struct k_pipe *output)
{
	K_OOPS(K_SYSCALL_OBJ(process, K_OBJ_PROCESS));
	K_OOPS(vrfy_argc(argc));
	K_OOPS(vrfy_argv_array(argc, argv));
	K_OOPS(vrfy_argv_strings(argc, argv));
	K_OOPS(vrfy_pipe(input));
	K_OOPS(vrfy_pipe(output));

	return z_impl_k_process_start(process, argc, argv, input, output);
}
#include <zephyr/syscalls/k_process_start_mrsh.c>

static inline int z_vrfy_k_process_stop(struct k_process *process)
{
	K_OOPS(K_SYSCALL_OBJ(process, K_OBJ_PROCESS));
	return z_impl_k_process_stop(process);
}
#include <zephyr/syscalls/k_process_stop_mrsh.c>
