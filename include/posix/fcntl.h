/*
 * Copyright (c) 2018 Juan Manuel Torres Palma <j.m.torrespalma@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FCNTL_H_
#define FCNTL_H_

/*
 * File or object creation flags, bitwise distinct.
 * According to open(), should be signed integers.
 */
#define O_CREAT   (0x01)
#define O_EXCL    (0x02)
#define O_NOCTTY  (0x04)
#define O_TRUNC   (0x08)

#include <sys/types.h>

#endif /* FCNTL_H_ */
