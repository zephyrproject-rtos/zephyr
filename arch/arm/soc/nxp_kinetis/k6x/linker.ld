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

/**
 * @file
 * @brief Linker command/script file
 *
 * This is the linker script for both standard images and XIP images.
 */

#include <autoconf.h>

/*
 * K64F Flash configuration fields
 * These are 16 bytes, which must be loaded to address 0x400, and include
 * default protection and security settings.
 * They are loaded at reset to various Flash Memory module (FTFE) registers.
 */

/*
 * No need to account for this when running a RAM-only image since that
 * security feature resides in ROM.
 */
#if defined(CONFIG_XIP)
	#define SKIP_TO_SECURITY_FRDM_K64F . = 0x400;
#endif

#include <arch/arm/cortex_m/scripts/linker.ld>
