/* receiver.h */

/*
 * Copyright (c) 1997-2010, 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
