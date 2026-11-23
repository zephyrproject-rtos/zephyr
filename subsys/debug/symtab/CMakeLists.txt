# Copyright (c) 2024 Meta Platforms
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  symtab.c
)

add_library(symtab
  symtab_stub.c
)

target_link_libraries(symtab zephyr_interface)
zephyr_library_link_libraries(symtab)
