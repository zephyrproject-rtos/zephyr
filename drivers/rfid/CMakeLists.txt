# Copyright (c) 2025 Conny Marco Menebröcker
#
# SPDX-License-Identifier: Apache-2.0
zephyr_library()
zephyr_library_sources(rfid_cr95hf.c)

add_compile_options(-save-temps)
