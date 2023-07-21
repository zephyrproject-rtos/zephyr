/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef Z_INCLUDE_SYSCALLS_KOBJECT_H
#define Z_INCLUDE_SYSCALLS_KOBJECT_H

#include <zephyr/kernel.h>

void k_object_access_grant(const void *object,
				     struct k_thread *thread);

#endif /* Z_INCLUDE_SYSCALLS_KOBJECT_H */