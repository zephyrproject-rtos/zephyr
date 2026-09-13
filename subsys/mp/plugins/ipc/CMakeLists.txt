#Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MP_PLUGIN_IPC mp_ipc_src.c mp_ipc_sink.c)