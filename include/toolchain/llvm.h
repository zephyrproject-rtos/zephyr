/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_

#include <toolchain/gcc.h>

#ifdef __clang__

#ifdef CONFIG_ARCH_POSIX

#if __has_feature(address_sanitizer)
#undef __no_sanitize_address
/*
 * Tell AddressSanitizer that it should not instrument the code.
 * This attribute can be set on a function or global variable.
 */
#define __no_sanitize_address __attribute__((no_sanitize("address")))
#endif /* __has_feature(address_sanitizer) */

#endif /* CONFIG_ARCH_POSIX */

#endif /* __clang__ */

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_LLVM_H_ */
