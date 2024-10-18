/**
 * @file
 * @brief ARC-specific constants for ELF binaries.
 *
 * References can be found here:
 * https://github.com/foss-for-synopsys-dwc-arc-processors/binutils
 */
/*
 * Copyright (c) 2024 Intel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARC_ELF_H
#define ZEPHYR_ARCH_ARC_ELF_H

#ifdef __cplusplus
extern "C" {
#endif

#define R_ARC_32    4
#define R_ARC_32_ME 27

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARC_ELF_H */
