/* linker.cmd - Linker command/script file */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
This is the linker script for both standard images and XIP images.
*/

/* Flash base address and size */
#define FLASH_START     0x40034000  /* Flash bank 1 */
#define FLASH_SIZE      152K

/*
 * SRAM base address and size
 *
 * Internal SRAM includes the exception vector table at reset, which is at
 * the beginning of the region.
 */
#define SRAM_START		CONFIG_RAM_START
#define SRAM_SIZE		CONFIG_RAM_SIZE

/* Data Closely Coupled Memory (DCCM) base address and size */
#define DCCM_START		0x80000000
#define DCCM_SIZE		8K


#include <arch/arc/v2/linker.cmd>
