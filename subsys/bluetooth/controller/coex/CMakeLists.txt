# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_COEX_TICKER coex_ticker.c
)
