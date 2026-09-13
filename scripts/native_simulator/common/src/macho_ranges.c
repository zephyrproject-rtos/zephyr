/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __APPLE__

/*
 * ld64 does not consume the GNU linker scripts which normally synthesize these
 * section boundary symbols for the native simulator final link. Re-export them
 * from the Mach-O section start/end pseudo-symbols so the existing runtime can
 * keep using the Linux-oriented names.
 */

#define MACHO_ALIAS(sym, target) \
	__asm__(".globl _" #sym "\n_" #sym " = " target)

/* Zephyr init entries */
MACHO_ALIAS(__init_EARLY_start, "section$start$__DATA$zi1_30_0");
MACHO_ALIAS(__init_PRE_KERNEL_1_start, "section$start$__DATA$zi1_30_0");
MACHO_ALIAS(__init_PRE_KERNEL_2_start, "section$start$__DATA$zi2_0_0");
MACHO_ALIAS(__init_POST_KERNEL_start, "section$start$__DATA$zi3_40_0");
MACHO_ALIAS(__init_APPLICATION_start, "section$start$__DATA$zi4_0_0");
MACHO_ALIAS(__init_end, "section$end$__DATA$zi4_90_0");
MACHO_ALIAS(__zephyr_init_array_start, "section$start$__DATA$zi1_30_0");
MACHO_ALIAS(__zephyr_init_array_end, "section$end$__DATA$zi4_90_0");

/* POSIX/native task hooks from the embedded image */
MACHO_ALIAS(__native_PRE_BOOT_1_tasks_start, "section$start$__DATA$natt0_0");
MACHO_ALIAS(__native_PRE_BOOT_2_tasks_start, "section$start$__DATA$natt1_10");
MACHO_ALIAS(__native_PRE_BOOT_3_tasks_start, "section$start$__DATA$natt4_1");
MACHO_ALIAS(__native_FIRST_SLEEP_tasks_start, "section$start$__DATA$natt4_1");
MACHO_ALIAS(__native_ON_EXIT_tasks_start, "section$start$__DATA$natt4_1");
MACHO_ALIAS(__native_tasks_end, "section$end$__DATA$natt4_1");

/* Runner-side NSI task hooks */
MACHO_ALIAS(__nsi_PRE_BOOT_1_tasks_start, "section$start$__DATA$nsit0_0");
MACHO_ALIAS(__nsi_PRE_BOOT_2_tasks_start, "section$start$__DATA$nsit1_0");
MACHO_ALIAS(__nsi_HW_INIT_tasks_start, "section$start$__DATA$nsit2_10");
MACHO_ALIAS(__nsi_PRE_BOOT_3_tasks_start, "section$start$__DATA$nsit5_100");
MACHO_ALIAS(__nsi_FIRST_SLEEP_tasks_start, "section$start$__DATA$nsit5_100");
MACHO_ALIAS(__nsi_ON_EXIT_PRE_tasks_start, "section$start$__DATA$nsit5_100");
MACHO_ALIAS(__nsi_ON_EXIT_POST_tasks_start, "section$start$__DATA$nsit6_0");
MACHO_ALIAS(__nsi_tasks_end, "section$end$__DATA$nsit6_0");

#endif /* __APPLE__ */
