/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
__ram_noinit$$Base = . ;
/* section.ram.noinit */
*(.ram_noinit)
__ram_noinit$$Limit = . ;
