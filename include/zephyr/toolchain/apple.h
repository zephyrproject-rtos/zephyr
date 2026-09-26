/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_APPLE_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_APPLE_H_

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_H_
#error Please do not include toolchain-specific headers directly, use <zephyr/toolchain.h> instead
#endif

/*
 * Mach-O has no equivalent of the ELF alias attribute, and its assembler can
 * only equate a symbol to one the linker can name. Emit the alias by hand and
 * give the aliased function external linkage through ALIAS_TARGET.
 */
#define FUNC_ALIAS(real_func, new_alias, return_type) \
	return_type new_alias(); \
	__asm__(".globl _" #new_alias "\n_" #new_alias " = _" #real_func)
#define FUNC_ALIAS_ARGS(real_func, new_alias, return_type, args) \
	return_type new_alias args; \
	__asm__(".globl _" #new_alias "\n_" #new_alias " = _" #real_func)

#undef ALIAS_TARGET
#define ALIAS_TARGET
#undef ALIAS_TARGET_INLINE
#define ALIAS_TARGET_INLINE

#include <zephyr/macho_iter_sections.h>

#define Z_MACHO_SEC_GET(token) _CONCAT(Z_MACHO_SEC_, token)
#define Z_MACHO_SECNAME(token) STRINGIFY(Z_MACHO_SEC_GET(token))

#define __GENERIC_SECTION(segment) \
	__attribute__((section("__DATA," STRINGIFY(segment))))
#define Z_GENERIC_SECTION(segment) __GENERIC_SECTION(segment)

#define __GENERIC_DOT_SECTION(segment) \
	__attribute__((section("__DATA,." STRINGIFY(segment))))
#define Z_GENERIC_DOT_SECTION(segment) __GENERIC_DOT_SECTION(segment)

/*
 * Mach-O section names are limited to 16 characters, so a section cannot be made
 * unique per file or per use the way the ELF backend does it: the file and
 * counter arguments are dropped and every use of one logical section lands in
 * the same output section. Ordering within a section is done with a linker order
 * file instead of by sorting section names.
 */
#define ___in_section(a, b, c) \
	__attribute__((section("__DATA," Z_MACHO_SECNAME(a))))
#define __in_section(a, b, c) ___in_section(a, b, c)

#define ___in_section_unique(a, b) \
	__attribute__((section("__DATA," Z_MACHO_SECNAME(a))))

#if defined(__clang__) && !defined(__OBJC__)
#undef __weak
#define __weak __attribute__((__weak__))
#elif !defined(__weak)
#define __weak __attribute__((__weak__))
#endif

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_APPLE_H_ */
