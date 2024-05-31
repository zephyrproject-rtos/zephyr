/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MLOG_H_
#define MLOG_H_

#include <stdio.h>

#define LOG_MODULE_REGISTER(x, y)

#define LOG_INF(fmt, args...) printf("I: " fmt "\n", ##args)
#define LOG_ERR(fmt, args...) printf("E: " fmt "\n", ##args)
#define LOG_DBG(fmt, args...) printf("D: " fmt "\n", ##args)

#endif
