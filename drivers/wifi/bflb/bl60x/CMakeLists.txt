# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

# -fcommon matches the blob build so duplicate weak symbol resolution
# works for its static cfg arrays.
zephyr_library_compile_options(-fcommon)

zephyr_library_sources(
  bflb_wifi_net.c
  bflb_wifi_ipc.c
  bflb_wifi_tx.c
  bflb_wifi_ccmp.c
  bflb_wifi_fw.c
  bflb_wifi_sae.c
  bflb_wifi_scan.c
  bflb_wifi_mgmt.c
  bflb_wifi_wpa_supp.c
)
