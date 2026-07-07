/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Place vendor blob .tcm_code.* sections into ITCM.
 * The lmac154 binary blob uses .tcm_code. for time-critical MAC code
 * that must execute from fast RAM.
 */

*(.tcm_code.*)
