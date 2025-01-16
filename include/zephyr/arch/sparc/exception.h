/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_SPARC_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_SPARC_EXCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct arch_esf {
	uint32_t out[8];
	uint32_t global[8];
	uint32_t psr;
	uint32_t pc;
	uint32_t npc;
	uint32_t wim;
	uint32_t tbr;
	uint32_t y;
};

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_SPARC_EXCEPTION_H_ */
