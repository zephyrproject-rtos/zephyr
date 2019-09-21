/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR__INCLUDE__ZPP__COMPILER_HPP
#define ZEPHYR__INCLUDE__ZPP__COMPILER_HPP

#ifndef __cplusplus
#error "ZPP is a C++ only library"
#else
#if __cplusplus < 201703L
#error "ZPP needs C++17 or newer"
#endif
#endif

#endif // ZEPHYR__INCLUDE__ZPP__COMPILER_HPP
