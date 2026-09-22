/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_APP_MEMORY_PARTITIONS_H_
#define ZEPHYR_INCLUDE_APP_MEMORY_PARTITIONS_H_

#ifdef CONFIG_USERSPACE
#include <zephyr/kernel.h> /* For struct k_mem_partition */

#if defined(CONFIG_MBEDTLS)
extern struct k_mem_partition k_mbedtls_partition;
#endif /* CONFIG_MBEDTLS */
#endif /* CONFIG_USERSPACE */
#endif /* ZEPHYR_INCLUDE_APP_MEMORY_PARTITIONS_H_ */
