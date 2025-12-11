/*
 * SPDX-FileCopyrightText: <text>Copyright 2021-2022, 2024-2025 Arm Limited and/or its
 * affiliates <open-source-office@arm.com></text>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_ETHOS_U_ETHOS_U_COMMON_H_
#define ZEPHYR_DRIVERS_MISC_ETHOS_U_ETHOS_U_COMMON_H_

#include <ethosu_driver.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ethosu_dts_info {
	void *base_addr;
	bool secure_enable;
	bool privilege_enable;
	void (*irq_config)(void);
	const void *fast_mem_base;
	size_t fast_mem_size;
};

struct ethosu_data {
	struct ethosu_driver drv;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_ETHOS_U_ETHOS_U_COMMON_H_ */
