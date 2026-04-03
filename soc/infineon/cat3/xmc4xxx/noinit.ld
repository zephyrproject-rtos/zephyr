/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 * Extracted from: infineon/XMCLib/devices/XMC4500/Source/GCC/XMC4500x1024.ld
 */

/* .no_init section contains chipid, SystemCoreClock and trimming data */
KEEP(*(.no_init))
