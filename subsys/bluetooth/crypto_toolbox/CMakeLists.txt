# SPDX-License-Identifier: Apache-2.0

# Create library "subsys__bluetooth__crypto_toolbox"
zephyr_library()

zephyr_include_directories(
  include
  )

zephyr_library_sources(
  crypto_toolbox_aes_cmac.c
  crypto_toolbox_f4.c
  crypto_toolbox_f5.c
  crypto_toolbox_f6.c
  crypto_toolbox_g2.c
  )

# zephyr_library_link_libraries(tinycrypt)
