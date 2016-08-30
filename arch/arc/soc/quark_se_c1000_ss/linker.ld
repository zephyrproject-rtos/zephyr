/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
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

/**
 * @brief Linker script for the Quark SE platform, both standard images and XIP
 * images.
 */

/* Flash base address and size */
#define FLASH_START     CONFIG_FLASH_BASE_ADDRESS  /* Flash bank 1 */
#define FLASH_SIZE      CONFIG_FLASH_SIZE

/*
 * SRAM base address and size
 *
 * Internal SRAM includes the exception vector table at reset, which is at
 * the beginning of the region.
 */
#define SRAM_START		CONFIG_SRAM_BASE_ADDRESS
#define SRAM_SIZE		CONFIG_SRAM_SIZE

/* Data Closely Coupled Memory (DCCM) base address and size */
#define DCCM_START		CONFIG_DCCM_BASE_ADDRESS
#define DCCM_SIZE		CONFIG_DCCM_SIZE

#include <arch/arc/v2/linker.ld>
