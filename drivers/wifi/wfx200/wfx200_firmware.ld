/*
 * Copyright (c) 2023 grandcentrix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

. = ALIGN(4);
__sl_wfx_firmware_start = . ;
KEEP(*(.sl_wfx_firmware))
__sl_wfx_firmware_end = . ;
