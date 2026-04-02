/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

__syscall int ext_syscall_ok(int a);

/*
 * This is a syscall that is intentionally not implemented. The build
 * syscall machinery will still pick it up and generate a stub for it,
 * that should be optmised away. For extensions, the symbol will end up
 * pointing to NULL. This is tested in the test_ext_syscall_fail test.
 */
__syscall void ext_syscall_fail(void);

#include <zephyr/syscalls/syscalls_ext.h>
