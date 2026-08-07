/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_APP_FORMATTER_PRINTK)
#include <zephyr/sys/printk.h>
#define PRINT_S "printk"
#define PRINT(...) printk(__VA_ARGS__)
#elif defined(CONFIG_APP_FORMATTER_PRINTF)
#include <stdio.h>
#ifdef CONFIG_NEWLIB_LIBC
#define PRINT_S "printf/newlib"
#else /* NEWLIB_LIBC */
#define PRINT_S "printf"
#endif /* NEWLIB_LIBC */
#define PRINT(...) printf(__VA_ARGS__)
#elif defined(CONFIG_APP_FORMATTER_PRINTFCB)
#include <zephyr/sys/cbprintf.h>
#ifdef CONFIG_NEWLIB_LIBC
#define PRINT_S "printfcb/newlib"
#else /* NEWLIB_LIBC */
#define PRINT_S "printfcb"
#endif /* NEWLIB_LIBC */
#define PRINT(...) printfcb(__VA_ARGS__)
#elif defined(CONFIG_APP_FORMATTER_FPRINTF)
#include <stdio.h>
#define PRINT_S "fprintf"
#define PRINT(...) fprintf(stdout, __VA_ARGS__)
#elif defined(CONFIG_APP_FORMATTER_FPRINTFCB)
#include <zephyr/sys/cbprintf.h>
#define PRINT_S "fprintfcb"
#define PRINT(...) fprintfcb(stdout, __VA_ARGS__)
#else
#error Unsupported configuration
#endif

int main(void)
{
	PRINT("Hello with %s on %s\nComplete\n", PRINT_S, CONFIG_BOARD);
	return 0;
}
