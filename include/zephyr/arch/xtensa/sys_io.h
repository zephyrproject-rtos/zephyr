/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_SYS_IO_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_SYS_IO_H_

#ifndef _ASMLANGUAGE

#ifdef CONFIG_XTENSA_SOC_HAS_CUSTOM_SYS_IO
#include <sys_io_soc.h>
#else
#include <zephyr/arch/common/sys_io.h>
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_SYS_IO_H_ */
