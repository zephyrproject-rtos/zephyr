/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 * Extracted from: infineon/XMCLib/devices/XMC1400/Source/GCC/XMC1400x0200.ld
 */

/* .no_init section contains chipid, SystemCoreClock and trimming data */
KEEP(*(.no_init))
