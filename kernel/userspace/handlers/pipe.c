/*
 * Copyright (c) 2024 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for kernel/pipe.c, compiled only with CONFIG_USERSPACE.
 */

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>

void z_vrfy_k_pipe_init(struct k_pipe *pipe, uint8_t *buffer, size_t buffer_size)
{
	K_OOPS(K_SYSCALL_OBJ_NEVER_INIT(pipe, K_OBJ_PIPE));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(buffer, buffer_size));

	z_impl_k_pipe_init(pipe, buffer, buffer_size);
}
#include <zephyr/syscalls/k_pipe_init_mrsh.c>

int z_vrfy_k_pipe_read(struct k_pipe *pipe, uint8_t *data, size_t len, k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(data, len));

	return z_impl_k_pipe_read(pipe, data, len, timeout);
}
#include <zephyr/syscalls/k_pipe_read_mrsh.c>

int z_vrfy_k_pipe_write(struct k_pipe *pipe, const uint8_t *data, size_t len, k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	K_OOPS(K_SYSCALL_MEMORY_READ(data, len));

	return z_impl_k_pipe_write(pipe, data, len, timeout);
}
#include <zephyr/syscalls/k_pipe_write_mrsh.c>

void z_vrfy_k_pipe_reset(struct k_pipe *pipe)
{
	K_OOPS(K_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	z_impl_k_pipe_reset(pipe);
}
#include <zephyr/syscalls/k_pipe_reset_mrsh.c>

void z_vrfy_k_pipe_close(struct k_pipe *pipe)
{
	K_OOPS(K_SYSCALL_OBJ(pipe, K_OBJ_PIPE));
	z_impl_k_pipe_close(pipe);
}
#include <zephyr/syscalls/k_pipe_close_mrsh.c>
