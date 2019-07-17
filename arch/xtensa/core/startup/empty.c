/*
 *  Copyright (c) 2019 Intel Corporation
 *
 *  SPDX-License-Identifier: Apache-2.0
 */
/*
 * See CMAKE_ASM_ARCHIVE_CREATE comment in:
 *  cmake/toolchain/zephyr/target.cmake
 *
 * This adds less than 2Kbytes to the file
 * xtensa/core/startup/libarch__xtensa__core__startup.a (less than 1Kb
 * with -g0) and makes no change to the final ELF image(s)
 */
typedef int placeholder;
