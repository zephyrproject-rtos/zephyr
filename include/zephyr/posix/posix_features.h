/*
 * Copyright (c) 2024 BayLibre SAS
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_POSIX_FEATURES_H_
#define ZEPHYR_INCLUDE_POSIX_POSIX_FEATURES_H_

#include <zephyr/autoconf.h>       /* CONFIG_* */
#include <zephyr/sys/util_macro.h> /* COND_CODE_1() */

/*
 * POSIX Application Environment Profiles (AEP - IEEE Std 1003.13-2003)
 */

#ifdef CONFIG_POSIX_AEP_REALTIME_MINIMAL
#define _POSIX_AEP_REALTIME_MINIMAL 200312L
#endif

#ifdef CONFIG_POSIX_AEP_REALTIME_CONTROLLER
#define _POSIX_AEP_REALTIME_CONTROLLER 200312L
#endif

#ifdef CONFIG_POSIX_AEP_REALTIME_DEDICATED
#define _POSIX_AEP_REALTIME_DEDICATED 200312L
#endif

/*
 * Subprofiling Considerations
 */
#define _POSIX_SUBPROFILE 1

/*
 * POSIX System Interfaces
 */

#define _POSIX_CHOWN_RESTRICTED (0)
#define _POSIX_NO_TRUNC         (0)
#define _POSIX_VDISABLE         ('\0')

/* _POSIX_ADVISORY_INFO: not supported */

/* _POSIX_JOB_CONTROL: not supported */

/* _POSIX_PRIORITIZED_IO: not supported */

/* _POSIX_REGEXP: not supported */
/* _POSIX_SAVED_IDS: not supported */

/* _POSIX_SHELL: not supported */
/* _POSIX_SPAWN: not supported */

/* _POSIX_SPORADIC_SERVER: not supported */

/* _POSIX_THREAD_PROCESS_SHARED: not supported */
/* _POSIX_THREAD_ROBUST_PRIO_INHERIT: not supported */
/* _POSIX_THREAD_ROBUST_PRIO_PROTECT: not supported */

/* _POSIX_THREAD_SPORADIC_SERVER: not supported */

/* _POSIX_TRACE: not supported */
/* _POSIX_TRACE_EVENT_FILTER: not supported */
/* _POSIX_TRACE_INHERIT: not supported */
/* _POSIX_TRACE_LOG: not supported */
/* _POSIX_TYPED_MEMORY_OBJECTS: not supported */

/*
 * POSIX v6 Options
 */
/* _POSIX_V6_ILP32_OFF32: not supported */
/* _POSIX_V6_ILP32_OFFBIG: not supported */
/* _POSIX_V6_LP64_OFF64: not supported */
/* _POSIX_V6_LPBIG_OFFBIG: not supported */

/*
 * POSIX v7 Options
 */
/* _POSIX_V7_ILP32_OFF32: not supported */
/* _POSIX_V7_ILP32_OFFBIG: not supported */
/* _POSIX_V7_LP64_OFF64: not supported */
/* _POSIX_V7_LPBIG_OFFBIG: not supported */

/*
 * POSIX2 Options
 */
/* _POSIX2_C_DEV: not supported */
/* _POSIX2_CHAR_TERM: not supported */
/* _POSIX2_FORT_DEV: not supported */
/* _POSIX2_FORT_RUN: not supported */
/* _POSIX2_LOCALEDEF: not supported */
/* _POSIX2_PBS: not supported */
/* _POSIX2_PBS_ACCOUNTING: not supported */
/* _POSIX2_PBS_CHECKPOINT: not supported */
/* _POSIX2_PBS_LOCATE: not supported */
/* _POSIX2_PBS_MESSAGE: not supported */
/* _POSIX2_PBS_TRACK: not supported */
/* _POSIX2_SW_DEV: not supported */
/* _POSIX2_UPE: not supported */

/*
 * X/Open System Interfaces
 */
/* _XOPEN_CRYPT: not supported */
/* _XOPEN_ENH_I18N: not supported */
#if defined(CONFIG_XSI_REALTIME) ||                                                                \
	(defined(CONFIG_POSIX_FSYNC) && defined(CONFIG_POSIX_MEMLOCK) &&                           \
	 defined(CONFIG_POSIX_MEMLOCK_RANGE) && defined(CONFIG_POSIX_MESSAGE_PASSING) &&           \
	 defined(CONFIG_POSIX_PRIORITY_SCHEDULING) &&                                              \
	 defined(CONFIG_POSIX_SHARED_MEMORY_OBJECTS) && defined(CONFIG_POSIX_SYNCHRONIZED_IO))
#define _XOPEN_REALTIME _XOPEN_VERSION
#endif
/* _XOPEN_REALTIME_THREADS: not supported */
/* _XOPEN_SHM: not supported */

/* _XOPEN_UNIX: not supported */
/* _XOPEN_UUCP: not supported */

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_FEATURES_H_ */
