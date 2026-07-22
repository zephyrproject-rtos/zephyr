# SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_DMX)
  zephyr_library()

  zephyr_library_sources(dmx_core.c)

  if(CONFIG_DMX_UART_ISR)
    zephyr_library_sources(dmx_uart_common.c)
  endif()

  zephyr_library_sources_ifdef(CONFIG_DMX_UART_ISR dmx_uart_isr.c)
endif()
