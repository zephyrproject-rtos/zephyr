/* receiver.h */

/*
 * Copyright (c) 1997-2010, 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RECEIVER_H
#define _RECEIVER_H

#include <zephyr.h>
#include "config.h"
#include "memcfg.h"

/* type defines. */
typedef struct {
	int count;
	unsigned int time;
	int size;
} GetInfo;

/* global data */
extern char data_recv[OCTET_TO_SIZEOFUNIT(MESSAGE_SIZE)];

#endif /* _RECEIVER_H */
