/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
__ramcode_from_flash$$Base = . ;
/* section.ram.code_from_flash */
*(.ram_code_from_flash)
. = ALIGN(4);
__ramcode_from_flash$$Limit = . ;
