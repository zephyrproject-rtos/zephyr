/**
 * @file cryptdev.c
 * Finds the crypto subsystem pseudodevice at boot
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_CRYPTDEV_H__
#define __BT_CRYPTDEV_H__

#include <crypto/cipher.h>
#include <device.h>

struct device *bt_get_cryptdev(void);

#endif /* __BT_CRYPTDEV_H__ */
