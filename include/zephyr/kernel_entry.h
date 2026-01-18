/*
 * Copyright (c) 2025 Baumer (www.baumer.com)
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_ENTRY_H_
#define ZEPHYR_INCLUDE_KERNEL_ENTRY_H_

#include <zephyr/toolchain.h>

/**
 * @defgroup kernel entry Kernel Entry
 * @ingroup kernel_apis
 * @{
 *
 * The k_entry is a label marking the very first assembly
 * instruction executed to set up the CPU basics such as stack pointer and
 * other CPU specific registers, memory protection, caches, etc.
 * often implemented in files called crt0.S, reset.S, entry.S, start.S or similar.
 *
 * k_entry is zephyr's common entry point symbol for all architectures,
 * storable in a pointer variable which may be made accessible to bootloaders.
 *
 * A bootloader shall be able, in a first stage, to jump to this symbol (address)
 * and leave the responsibility of setting up the CPU correctly to the subsequent
 * reset/entry/start assembly code. In further stages that assembly code needs
 * to setup the C environment and enter the kernel.
 *
 */

#if !defined(_ASMLANGUAGE)

#ifdef __cplusplus
extern "C" {
#endif

void k_entry(void);

#ifdef __cplusplus
}
#endif


#else
GTEXT(k_entry)
#endif /* _ASMLANGUAGE */


/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_KERNEL_VERSION_H_ */
