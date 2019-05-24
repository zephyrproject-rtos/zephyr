/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_APP_MEMORY_PARTITIONS_H
#define ZEPHYR_APP_MEMORY_PARTITIONS_H

#ifdef CONFIG_USERSPACE
#include <kernel.h> /* For struct k_mem_partition */

#if defined(CONFIG_MBEDTLS)
extern struct k_mem_partition k_mbedtls_partition;
#endif /* CONFIG_MBEDTLS */
#endif /* CONFIG_USERSPACE */
#endif /* ZEPHYR_APP_MEMORY_PARTITIONS_H */
