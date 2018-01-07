/*
 * Boot banner handling
 */

#ifndef __BOOT_H
#define __BOOT_H

/* kernel build timestamp items */

#define BUILD_TIMESTAMP __DATE__ " " __TIME__

#ifdef CONFIG_BUILD_TIMESTAMP
const char * const build_timestamp = BUILD_TIMESTAMP;
#endif

/* boot banner items */

#ifdef CONFIG_BOOT_BANNER
const char * const kernel_version = KERNEL_VERSION_STRING;
#endif

#if defined(CONFIG_BOOT_DELAY) && CONFIG_BOOT_DELAY > 0
static const unsigned int boot_delay = CONFIG_BOOT_DELAY;
#else
static const unsigned int boot_delay;
#endif

#endif
