# Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
# SPDX-License-Identifier: Apache-2.0

set(psa_header ${ZEPHYR_BASE}/include/zephyr/call_offloader/services/psa.h)

zephyr_library()
zephyr_library_sources(psa.c)
zephyr_library_compile_definitions(CALL_OFFLOADER_PSA_NO_REDIRECT)

zephyr_compile_options("SHELL:$<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,imacros> ${psa_header}>")
