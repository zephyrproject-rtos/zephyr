# SPDX-License-Identifier: Apache-2.0

zephyr_sources(cpp_init.c)

zephyr_sources_ifdef(CONFIG_CPP_STATIC_INIT_GNU
  cpp_init_array.c
  cpp_ctors.c
  cpp_dtors.c
)
