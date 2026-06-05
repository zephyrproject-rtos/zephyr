/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Collect the vendor blob's .tcm_code.* sections into .data.
 * The lmac154 binary blob uses .tcm_code. for time-critical MAC code that
 * must execute from RAM. BL61x has no ITCM, so it runs from OCRAM instead.
 * Without this the linker places those sections as orphans.
 */

*(.tcm_code .tcm_code.*)
