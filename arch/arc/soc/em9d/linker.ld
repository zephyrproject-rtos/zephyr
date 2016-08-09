/*
 * Copyright (c) 2016 Synopsys, Inc. All rights reserved.
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
 * @brief Linker script for the Synopsys EM Starterkit v2.2 EM9D platform.
 */

/*
 * ICCM base address and size
 *
 * ICCM includes the exception vector table at reset, which is at
 * the beginning of the region.
 */

/* Instruction Closely Coupled Memory (ICCM) base address and size */
#define ICCM_START		CONFIG_ICCM_BASE_ADDRESS
#define ICCM_SIZE		CONFIG_ICCM_SIZE

/*
 * DCCM base address and size. DCCM is the data memory.
 */
/* Data Closely Coupled Memory (DCCM) base address and size */
#define DCCM_START		CONFIG_DCCM_BASE_ADDRESS
#define DCCM_SIZE		CONFIG_DCCM_SIZE

#include <arch/arc/v2/linker.ld>

