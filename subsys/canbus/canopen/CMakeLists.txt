# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
  canopen_core.c
  canopen_emcy.c
  canopen_hb.c
  canopen_nmt.c
  canopen_od.c
  canopen_od_comm.c
  canopen_pdo.c
  canopen_sdo_server.c
  canopen_sync.c
)
zephyr_library_sources_ifdef(CONFIG_CANOPEN_TIME canopen_time.c)
