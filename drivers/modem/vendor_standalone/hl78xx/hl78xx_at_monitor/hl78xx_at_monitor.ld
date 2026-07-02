/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Netfeasa Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

. = ALIGN(4);
_hl78xx_at_monitor_entry_list_start = .;
KEEP(*(SORT_BY_NAME("._hl78xx_at_monitor_entry.*")));
_hl78xx_at_monitor_entry_list_end = .;
