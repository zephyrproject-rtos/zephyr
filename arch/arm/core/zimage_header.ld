/*
 * Copyright (c) 2023, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_ARM_ZIMAGE_HEADER)
KEEP(*(.image_header))
KEEP(*(.".image_header.*"))
__end = .;
#endif
