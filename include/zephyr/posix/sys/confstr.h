/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief POSIX configuration string name declarations.
 * @ingroup posix
 *
 * Defines the @c _CS_* name constants used as the first argument to
 * confstr() to query implementation-defined configuration strings.
 */

#ifndef ZEPHYR_INCLUDE_POSIX_SYS_CONFSTR_H_
#define ZEPHYR_INCLUDE_POSIX_SYS_CONFSTR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief confstr() name constants.
 */
enum {
	/**
	 * @brief Value for the PATH environment variable that finds all standard utilities.
	 */
	_CS_PATH,
	/**
	 * @brief C compiler flags for the ILP32 model with 32-bit @c off_t (POSIX.1-2008).
	 */
	_CS_POSIX_V7_ILP32_OFF32_CFLAGS,
	/**
	 * @brief Linker flags for the ILP32 model with 32-bit @c off_t (POSIX.1-2008).
	 */
	_CS_POSIX_V7_ILP32_OFF32_LDFLAGS,
	/**
	 * @brief Libraries for the ILP32 model with 32-bit @c off_t (POSIX.1-2008).
	 */
	_CS_POSIX_V7_ILP32_OFF32_LIBS,
	/**
	 * @brief C compiler flags for the ILP32 model with 64-bit or larger @c off_t
	 * (POSIX.1-2008).
	 */
	_CS_POSIX_V7_ILP32_OFFBIG_CFLAGS,
	/**
	 * @brief Linker flags for the ILP32 model with 64-bit or larger @c off_t (POSIX.1-2008).
	 */
	_CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS,
	/**
	 * @brief Libraries for the ILP32 model with 64-bit or larger @c off_t (POSIX.1-2008).
	 */
	_CS_POSIX_V7_ILP32_OFFBIG_LIBS,
	/**
	 * @brief C compiler flags for the LP64 model with 64-bit @c off_t (POSIX.1-2008).
	 */
	_CS_POSIX_V7_LP64_OFF64_CFLAGS,
	/**
	 * @brief Linker flags for the LP64 model with 64-bit @c off_t (POSIX.1-2008).
	 */
	_CS_POSIX_V7_LP64_OFF64_LDFLAGS,
	/**
	 * @brief Libraries for the LP64 model with 64-bit @c off_t (POSIX.1-2008).
	 */
	_CS_POSIX_V7_LP64_OFF64_LIBS,
	/**
	 * @brief C compiler flags for the LPBIG model with 64-bit or larger @c off_t
	 * (POSIX.1-2008).
	 */
	_CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS,
	/**
	 * @brief Linker flags for the LPBIG model with 64-bit or larger @c off_t (POSIX.1-2008).
	 */
	_CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS,
	/**
	 * @brief Libraries for the LPBIG model with 64-bit or larger @c off_t (POSIX.1-2008).
	 */
	_CS_POSIX_V7_LPBIG_OFFBIG_LIBS,
	/**
	 * @brief C compiler flags for building multi-threaded applications (POSIX.1-2008).
	 */
	_CS_POSIX_V7_THREADS_CFLAGS,
	/**
	 * @brief Linker flags for building multi-threaded applications (POSIX.1-2008).
	 */
	_CS_POSIX_V7_THREADS_LDFLAGS,
	/**
	 * @brief Newline-separated list of width-restricted programming environments
	 * (POSIX.1-2008).
	 */
	_CS_POSIX_V7_WIDTH_RESTRICTED_ENVS,
	/**
	 * @brief Environment variable settings required for a conforming POSIX.1-2008 environment.
	 */
	_CS_V7_ENV,
	/**
	 * @brief C compiler flags for the ILP32 model with 32-bit @c off_t (POSIX.1-2001).
	 */
	_CS_POSIX_V6_ILP32_OFF32_CFLAGS,
	/**
	 * @brief Linker flags for the ILP32 model with 32-bit @c off_t (POSIX.1-2001).
	 */
	_CS_POSIX_V6_ILP32_OFF32_LDFLAGS,
	/**
	 * @brief Libraries for the ILP32 model with 32-bit @c off_t (POSIX.1-2001).
	 */
	_CS_POSIX_V6_ILP32_OFF32_LIBS,
	/**
	 * @brief C compiler flags for the ILP32 model with 64-bit or larger @c off_t
	 * (POSIX.1-2001).
	 */
	_CS_POSIX_V6_ILP32_OFFBIG_CFLAGS,
	/**
	 * @brief Linker flags for the ILP32 model with 64-bit or larger @c off_t (POSIX.1-2001).
	 */
	_CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS,
	/**
	 * @brief Libraries for the ILP32 model with 64-bit or larger @c off_t (POSIX.1-2001).
	 */
	_CS_POSIX_V6_ILP32_OFFBIG_LIBS,
	/**
	 * @brief C compiler flags for the LP64 model with 64-bit @c off_t (POSIX.1-2001).
	 */
	_CS_POSIX_V6_LP64_OFF64_CFLAGS,
	/**
	 * @brief Linker flags for the LP64 model with 64-bit @c off_t (POSIX.1-2001).
	 */
	_CS_POSIX_V6_LP64_OFF64_LDFLAGS,
	/**
	 * @brief Libraries for the LP64 model with 64-bit @c off_t (POSIX.1-2001).
	 */
	_CS_POSIX_V6_LP64_OFF64_LIBS,
	/**
	 * @brief C compiler flags for the LPBIG model with 64-bit or larger @c off_t
	 * (POSIX.1-2001).
	 */
	_CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS,
	/**
	 * @brief Linker flags for the LPBIG model with 64-bit or larger @c off_t (POSIX.1-2001).
	 */
	_CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS,
	/**
	 * @brief Libraries for the LPBIG model with 64-bit or larger @c off_t (POSIX.1-2001).
	 */
	_CS_POSIX_V6_LPBIG_OFFBIG_LIBS,
	/**
	 * @brief Newline-separated list of width-restricted programming environments
	 * (POSIX.1-2001).
	 */
	_CS_POSIX_V6_WIDTH_RESTRICTED_ENVS,
	/**
	 * @brief Environment variable settings required for a conforming POSIX.1-2001 environment.
	 */
	_CS_V6_ENV,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SYS_CONFSTR_H_ */
