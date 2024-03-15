/*
 * Copyright (c) 2023 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARC_INCLUDE_ERR_DUMP_HANDLING_H_
#define ZEPHYR_ARCH_ARC_INCLUDE_ERR_DUMP_HANDLING_H_

#if defined CONFIG_LOG
#define ARC_EXCEPTION_DUMP(...) LOG_ERR(__VA_ARGS__)
#else
#define ARC_EXCEPTION_DUMP(format, ...) printk(format "\n", ##__VA_ARGS__)
#endif

#endif /* ZEPHYR_ARCH_ARC_INCLUDE_ERR_DUMP_HANDLING_H_ */
