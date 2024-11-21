/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal configuration for LZMA support in MCUboot with Zephyr on RISC-V
 */

#ifndef LZMA_CONFIG_H
#define LZMA_CONFIG_H

/* Define to 1 to enable mythread */
#define MYTHREAD_ENABLED 1

/* Define to 1 if crc32 integrity check is enabled. */
#define HAVE_CHECK_CRC32 1

/* Define to 1 if the LZMA decoders are enabled. */
#define HAVE_DECODERS 1

/* Enable only the essential decoders. */
#define HAVE_DECODER_LZMA1 1

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if the system has the type `_Bool`. */
#define HAVE__BOOL 1

/* Define to 1 to disable debugging code for smaller footprint. */
#define NDEBUG 1

/* Define to 1 if optimizing for size */
#define HAVE_SMALL 1

#endif /* LZMA_CONFIG_H */
