/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_HAL_NXP_MCUX_MCUX_SDK_NG_MCUX_PORT_ZEPHYR_H_
#define ZEPHYR_MODULES_HAL_NXP_MCUX_MCUX_SDK_NG_MCUX_PORT_ZEPHYR_H_

#include <zephyr/linker/section_tags.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_MCUX_NONCACHEABLE_NOCACHE)
#define AT_NONCACHEABLE_SECTION(var) var __nocache
#define AT_NONCACHEABLE_SECTION_ALIGN(var, alignbytes) \
	var __nocache __aligned(alignbytes)
#define AT_NONCACHEABLE_SECTION_INIT(var) var __nocache_load
#define AT_NONCACHEABLE_SECTION_ALIGN_INIT(var, alignbytes) \
	var __nocache_load __aligned(alignbytes)
#endif

#endif /* ZEPHYR_MODULES_HAL_NXP_MCUX_MCUX_SDK_NG_MCUX_PORT_ZEPHYR_H_ */
