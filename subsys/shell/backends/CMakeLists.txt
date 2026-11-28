# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_SERIAL
  shell_uart.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_DUMMY
  shell_dummy.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_RTT
  shell_rtt.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_SSH
  shell_ssh.c
)

if(CONFIG_SHELL_BACKEND_SSH AND CONFIG_MBEDTLS_BUILTIN)
  # shell_ssh.c is built into the "zephyr" library.
  target_link_libraries(zephyr PUBLIC tfpsacrypto)
endif()

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_TELNET
  shell_telnet.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_MQTT
  shell_mqtt.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_WEBSOCKET
  shell_websocket.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_RPMSG
  shell_rpmsg.c
)

zephyr_sources_ifdef(
  CONFIG_SHELL_BACKEND_ADSP_MEMORY_WINDOW
  shell_adsp_memory_window.c
)
