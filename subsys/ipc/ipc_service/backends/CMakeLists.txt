# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_ICMSG		ipc_icmsg.c)
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_RPMSG		ipc_rpmsg_static_vrings.c)
