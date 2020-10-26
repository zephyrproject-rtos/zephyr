/*
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DT_UTIL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DT_UTIL_H_

/*
 * This file exists to keep in-tree DTS clean.  This means, only
 * #include <dt-bindings/foo.h> form should be included.  The
 * <dt-bindings/dt-util.h> wraps <sys/util_macro.h> file exposing
 * all macro base definitions to DTS preprocessor.  It provides
 * necessary background to elaborate complex constructions like
 * variable length macros with zero or more elements.
 */

#include <sys/util_macro.h>

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DT_UTIL_H_ */
