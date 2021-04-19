/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_

#ifndef _LINKER
#if defined(_ASMLANGUAGE)

#include <toolchain/common.h>

#else /* defined(_ASMLANGUAGE) */

#define __no_optimization __attribute__((optnone))

#include <toolchain/gcc.h>
#endif /* _ASMLANGUAGE */

#endif /* !_LINKER */

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_ */
