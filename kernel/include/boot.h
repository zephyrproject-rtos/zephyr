/*
 * Boot banner handling
 */

#ifndef __BOOT_H
#define __BOOT_H

/* kernel build timestamp items */

#define BUILD_TIMESTAMP "BUILD: " __DATE__ " " __TIME__

#ifdef CONFIG_BUILD_TIMESTAMP
const char * const build_timestamp = BUILD_TIMESTAMP;
#endif

/* boot banner items */

static const unsigned int boot_delay;
#if defined(CONFIG_BOOT_DELAY) && CONFIG_BOOT_DELAY > 0
#define BOOT_DELAY_BANNER " (delayed boot "	\
	STRINGIFY(CONFIG_BOOT_DELAY) "ms)"
static const unsigned int boot_delay = CONFIG_BOOT_DELAY;
#else
#define BOOT_DELAY_BANNER ""
static const unsigned int boot_delay;
#endif
#define BOOT_BANNER "BOOTING ZEPHYR OS v"	\
	KERNEL_VERSION_STRING BOOT_DELAY_BANNER

#if !defined(CONFIG_BOOT_BANNER)
#define PRINT_BOOT_BANNER() do { } while (0)
#elif !defined(CONFIG_BUILD_TIMESTAMP)
#define PRINT_BOOT_BANNER() printk("***** " BOOT_BANNER " *****\n")
#else
#define PRINT_BOOT_BANNER() \
	printk("***** " BOOT_BANNER " - %s *****\n", build_timestamp)
#endif

#endif
