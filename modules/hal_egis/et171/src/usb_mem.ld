/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_XIP
SECTION_DATA_PROLOGUE(.usb_mem,(NOLOAD),)
{
	KEEP(*(.usb_mem ))
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
#else
KEEP(*(.usb_mem))
#endif
