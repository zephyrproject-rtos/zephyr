# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${ZEPHYR_BASE}/subsys/lorawan/nvm
)

zephyr_library_sources(
  lorawan.c
  engine.c
  mac/mac.c
  mac/join.c
  mac/mac_commands.c
  mac/send.c
  radio.c
)

zephyr_library_sources_ifndef(CONFIG_LORAWAN_NVM_NONE nvm.c)

add_subdirectory(crypto)
add_subdirectory(region)
