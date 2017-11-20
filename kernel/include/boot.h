/*
 * Boot banner handling
 */

#ifndef __BOOT_H
#define __BOOT_H

#include <board_meta.h>

/* kernel build timestamp items */

#define BUILD_TIMESTAMP "BUILD: " __DATE__ " " __TIME__

#ifdef CONFIG_BUILD_TIMESTAMP
const char *const build_timestamp = BUILD_TIMESTAMP;
#endif

/* boot banner items */

static const unsigned int boot_delay;
#if defined(CONFIG_BOOT_DELAY) && CONFIG_BOOT_DELAY > 0
#define BOOT_DELAY_BANNER " (delayed boot " \
	STRINGIFY(CONFIG_BOOT_DELAY) "ms)"
static const unsigned int boot_delay = CONFIG_BOOT_DELAY;
#else
#define BOOT_DELAY_BANNER ""
static const unsigned int boot_delay;
#endif
#define BOOT_BANNER "BOOTING ZEPHYR OS v" \
	KERNEL_VERSION_STRING BOOT_DELAY_BANNER

#if !defined(CONFIG_BOOT_BANNER)
#define PRINT_BOOT_BANNER() do { } while (0)
#elif !defined(CONFIG_BUILD_TIMESTAMP)
#define PRINT_BOOT_BANNER() printk("***** " BOOT_BANNER " *****\n")
#else
#define PRINT_BOOT_BANNER() \
	printk("***** " BOOT_BANNER " - %s *****\n", build_timestamp)
#endif

#if defined(CONFIG_SYSTEM_INFO)
#if defined(CONFIG_SOC_SERIES) && defined(CONFIG_SOC_FAMILY)
#define PRINT_SOC_INFO() do {					    \
		printk(" CPU/SoC Family: %s\n", CONFIG_SOC_FAMILY); \
		printk(" CPU/SoC Series: %s\n", CONFIG_SOC_SERIES); \
} while (0)
#else
#define PRINT_SOC_INFO() do { } while (0)
#endif


#ifdef META_BOARD_NAME
#define SYS_BOARD_NAME META_BOARD_NAME
#else
#define SYS_BOARD_NAME CONFIG_BOARD
#endif

#define PRINT_SYSTEM_INFO() do {					   \
		printk("System information:\n Board: %s\n", SYS_BOARD_NAME); \
		PRINT_SOC_INFO();					   \
		printk("Architecture: %s\n", CONFIG_ARCH);		   \
		printk("******************\n\n");			   \
} while (0)
#else
#define PRINT_SYSTEM_INFO() do { } while (0)
#endif


#endif
