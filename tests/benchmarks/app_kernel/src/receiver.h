/* receiver.h */

/*
 * Copyright (c) 1997-2010, 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RECEIVER_H
#define _RECEIVER_H

#include <zephyr/kernel.h>
#include "config.h"
#include "memcfg.h"
#include "master.h"

/* type defines. */
struct getinfo{
	int count;
	unsigned int time;
	int size;
};

/* global data */
extern char data_recv[MESSAGE_SIZE];

#endif /* _RECEIVER_H */
