/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
__ram_from_flash$$Base = . ;
/* section.ram.from_flash */
*(.ram_from_flash)
. = ALIGN(4);
__ram_from_flash$$Limit = . ;
