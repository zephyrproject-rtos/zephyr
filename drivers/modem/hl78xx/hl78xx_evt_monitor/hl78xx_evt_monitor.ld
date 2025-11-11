/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*HL78XX event monitors */
. = ALIGN(4);
_hl78xx_evt_monitor_entry_list_start = .;
KEEP(*(SORT_BY_NAME("._hl78xx_evt_monitor_entry.*")));
_hl78xx_evt_monitor_entry_list_end = .;
