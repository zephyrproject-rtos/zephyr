# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS_BUILTIN tfpsacrypto)

zephyr_library_sources(
  lorawan.c
  engine.c
  mac/mac.c
  mac/join.c
  mac/mac_commands.c
  mac/send.c
  radio.c
)

add_subdirectory(crypto)
add_subdirectory(region)
