# SPDX-License-Identifier: Apache-2.0

# libraries
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_RPMSG			ipc_rpmsg.c)
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_STATIC_VRINGS		ipc_static_vrings.c)

# backends
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_RPMSG_MI	ipc_rpmsg_static_vrings_mi.c)
