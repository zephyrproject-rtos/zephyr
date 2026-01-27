#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# Protocol API is only exposed to MCUmgr internals.
zephyr_library_named(mcumgr_smp)
zephyr_library_sources(src/smp.c)
zephyr_library_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_FORWARD_TREE
  src/forward_tree.c
)
