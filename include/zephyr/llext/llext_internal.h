/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_INTERNAL_H
#define ZEPHYR_LLEXT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Private header for linkable loadable extensions
 */

/** @cond ignore */

struct llext_loader;
struct llext;

const void *llext_loaded_sect_ptr(struct llext_loader *ldr, struct llext *ext, unsigned int sh_ndx);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_INTERNAL_H */
