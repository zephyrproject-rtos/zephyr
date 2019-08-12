/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DEPRECATE_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DEPRECATE_H_

#if defined(__GNUC__)
#define __DT_DEPRECATED_MACRO _Pragma("GCC warning \"DTS Macro is deprecated\"")
#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DEPRECATE_H_ */
