# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MODEM_QUECTEL_BC660K OR CONFIG_MODEM_QUECTEL_BC66)
  zephyr_library_sources(bc66x/bc66x.c)
  zephyr_library_sources(bc66x/bc66x_net.c)
  zephyr_library_sources(bc66x/bc66x_hw.c)
  zephyr_library_sources(bc66x/bc66x_utils.c)
endif()
