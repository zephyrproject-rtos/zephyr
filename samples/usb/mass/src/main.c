/*
 * Copyright (c) 2016 Intel Corporation.
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

/* Sample to put the device in USB mass storage mode backed on a 16k RAMDisk. */

#include <zephyr.h>
#include <misc/sys_log.h>

void main(void)
{
	/* Nothing to be done other than the selecting appropriate build
	 * config options. Everything is driven from the USB host side.
	 */
	SYS_LOG_INF("The device is put in USB mass storage mode.\n");
}

