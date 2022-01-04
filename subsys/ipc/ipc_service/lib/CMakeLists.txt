# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_IPC_SERVICE_RPMSG			ipc_rpmsg.c)
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_STATIC_VRINGS		ipc_static_vrings.c)
