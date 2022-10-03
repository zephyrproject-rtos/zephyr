# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_ADSP
  log_backend_adsp.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_ADSP_HDA
  log_backend_adsp_hda.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_ADSP_MTRACE
  log_backend_adsp_mtrace.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_EFI_CONSOLE
  log_backend_efi_console.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_FS
  log_backend_fs.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_NATIVE_POSIX
  log_backend_native_posix.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_NET
  log_backend_net.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_RTT
  log_backend_rtt.c
)

zephyr_include_directories_ifdef(
  CONFIG_LOG_BACKEND_SPINEL
  ${ZEPHYR_BASE}/subsys/net/lib/openthread/platform/
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_SPINEL
  log_backend_spinel.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_SWO
  log_backend_swo.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_UART
  log_backend_uart.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_BACKEND_XTENSA_SIM
  log_backend_xtensa_sim.c
)
