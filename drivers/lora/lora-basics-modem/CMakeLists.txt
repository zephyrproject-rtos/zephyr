# SPDX-License-Identifier: Apache-2.0

# LoRa drivers depend on the include directories exposed by the lora-basics-modem
# library. Hence, if it exists then the source files are added to that otherwise
# a library with same name is created.
if(TARGET lora-basics-modem)
  set(ZEPHYR_CURRENT_LIBRARY lora-basics-modem)
else()
  zephyr_library_named(lora-basics-modem)
endif()

zephyr_include_directories(.)

zephyr_library_sources(lbm_common.c)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_LORA_RYLRXXX ../rylrxxx.c)
zephyr_library_sources_ifdef(CONFIG_LORA_SX126X lbm_sx126x.c)
zephyr_library_sources_ifdef(CONFIG_LORA_SX127X lbm_sx127x.c)
# zephyr-keep-sorted-stop
