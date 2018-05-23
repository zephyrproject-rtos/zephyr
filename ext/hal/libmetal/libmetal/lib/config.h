/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	config.h
 * @brief	Generated configuration settings for libmetal.
 */

#ifndef __METAL_CONFIG__H__
#define __METAL_CONFIG__H__

#ifdef __cplusplus
extern "C" {
#endif

/** Library major version number. */
#define METAL_VER_MAJOR		@PROJECT_VER_MAJOR@

/** Library minor version number. */
#define METAL_VER_MINOR		@PROJECT_VER_MINOR@

/** Library patch level. */
#define METAL_VER_PATCH		@PROJECT_VER_PATCH@

/** Library version string. */
#define METAL_VER		"@PROJECT_VER@"

/** System type (linux, generic, ...). */
#define METAL_SYSTEM		"@PROJECT_SYSTEM@"
#define METAL_SYSTEM_@PROJECT_SYSTEM_UPPER@

/** Processor type (arm, x86_64, ...). */
#define METAL_PROCESSOR		"@PROJECT_PROCESSOR@"
#define METAL_PROCESSOR_@PROJECT_PROCESSOR_UPPER@

/** Machine type (zynq, zynqmp, ...). */
#define METAL_MACHINE		"@PROJECT_MACHINE@"
#define METAL_MACHINE_@PROJECT_MACHINE_UPPER@

#cmakedefine HAVE_STDATOMIC_H
#cmakedefine HAVE_FUTEX_H

#ifdef __cplusplus
}
#endif

#endif /* __METAL_CONFIG__H__ */
