/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Pull the SEGGER RTT globals into the Zephyr nocache section so they
 * are MPU-marked non-cacheable. Requires CONFIG_SEGGER_RTT_SECTION_NONE=y
 * so SEGGER source leaves these in default .bss subsections under
 * -fdata-sections.
 */
*(.bss._SEGGER_RTT*)
*(.bss._acUpBuffer*)
*(.bss._acDownBuffer*)
