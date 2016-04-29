/*
 * Copyright (c) 2011-2015, Wind River Systems, Inc.
 * Copyright (c) 2016, Intel Corporation
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

#include <nanokernel.h>
#include "board.h"
#include <uart.h>
#include <device.h>
#include <init.h>

#ifdef CONFIG_VERSION_HEADER
#include "version_header.h"
/* The content of this struct is overwritten in a post-build script */
struct version_header version_header __attribute__((section(".version_header")))
__attribute__((used)) = {
	.magic = {'$', 'B', '!', 'N'},
	.version = 0x01,
	.reserved_1 = {0, 0, 0, 0},
	.reserved_2 = {0, 0, 0, 0},
};
#endif
