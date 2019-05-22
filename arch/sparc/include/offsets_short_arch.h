/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_SPARC_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_SPARC_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

#define _esf_reg(reg)	\
	(__z_arch_esf_t_## reg ##_OFFSET)

#endif /* ZEPHYR_ARCH_SPARC_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
