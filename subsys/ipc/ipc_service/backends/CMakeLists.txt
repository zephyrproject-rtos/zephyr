# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_ICMSG			ipc_icmsg.c)
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_INITIATOR	ipc_icmsg_me_initiator.c)
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_ICMSG_ME_FOLLOWER	ipc_icmsg_me_follower.c)
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_ICBMSG			ipc_icbmsg.c)
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_INTEL_ADSP_HOST_IPC	ipc_intel_adsp_host_ipc.c)
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_RPMSG			ipc_rpmsg_static_vrings.c)
zephyr_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE			ipc_rpmsg_lite.c)

if(CONFIG_IPC_SERVICE_BACKEND_LINK)
  # The backend and its transports share the state of the links and see the protocol core
  zephyr_library_named(ipc_service_link)
  zephyr_library_sources(ipc_link.c)
  zephyr_library_sources_ifdef(CONFIG_IPC_SERVICE_BACKEND_LINK_UART ipc_link_uart.c)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/ipc/ipc_service/lib)
endif()
