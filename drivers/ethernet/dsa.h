/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *               Lukasz Majewski <lukma@denx.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DSA_H__
#define __DSA_H__

#include <device.h>
#include <drivers/spi.h>

extern int dsa_init(struct device *dev);

#endif /* __DSA_H__ */
