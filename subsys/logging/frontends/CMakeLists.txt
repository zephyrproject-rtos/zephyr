# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(
  CONFIG_LOG_FRONTEND_DICT_UART
  log_frontend_dict_uart.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_FRONTEND_STMESP
  log_frontend_stmesp.c
)

zephyr_sources_ifdef(
  CONFIG_LOG_FRONTEND_STMESP_DEMUX
  log_frontend_stmesp_demux.c
)

zephyr_include_directories_ifdef(
  CONFIG_LOG_FRONTEND_STMESP_FSC
  stmesp
)
