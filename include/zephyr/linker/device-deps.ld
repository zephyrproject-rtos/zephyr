/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

__device_deps_start = .;
#ifdef LINKER_DEVICE_DEPS_PASS1
KEEP(*(SORT(.__device_deps_pass1*)));
#else
KEEP(*(SORT(.__device_deps_pass2*)));
#endif /* LINKER_DEVICE_DEPS_PASS1 */
__device_deps_end = .;
