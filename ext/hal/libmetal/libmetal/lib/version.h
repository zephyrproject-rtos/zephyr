/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	version.h
 * @brief	Library version information for libmetal.
 */

#ifndef __METAL_VERSION__H__
#define __METAL_VERSION__H__

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup versions Library Version Interfaces
 *  @{ */

/**
 *  @brief	Library major version number.
 *
 *  Return the major version number of the library linked into the application.
 *  This is required to match the value of METAL_VER_MAJOR, which is the major
 *  version of the library that the application was compiled against.
 *
 *  @return	Library major version number.
 *  @see	METAL_VER_MAJOR
 */
extern int metal_ver_major(void);

/**
 *  @brief	Library minor version number.
 *
 *  Return the minor version number of the library linked into the application.
 *  This could differ from the value of METAL_VER_MINOR, which is the minor
 *  version of the library that the application was compiled against.
 *
 *  @return	Library minor version number.
 *  @see	METAL_VER_MINOR
 */
extern int metal_ver_minor(void);

/**
 *  @brief	Library patch level.
 *
 *  Return the patch level of the library linked into the application.  This
 *  could differ from the value of METAL_VER_PATCH, which is the patch level of
 *  the library that the application was compiled against.
 *
 *  @return	Library patch level.
 *  @see	METAL_VER_PATCH
 */
extern int metal_ver_patch(void);

/**
 *  @brief	Library version string.
 *
 *  Return the version string of the library linked into the application.  This
 *  could differ from the value of METAL_VER, which is the version string of
 *  the library that the application was compiled against.
 *
 *  @return	Library version string.
 *  @see	METAL_VER
 */
extern const char *metal_ver(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_VERSION__H__ */
