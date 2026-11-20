# Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_OFFLOADER engine.c)
zephyr_sources_ifdef(CONFIG_OFFLOADER_PSA psa.c)

zephyr_library_compile_definitions_ifdef(CONFIG_OFFLOADER_PSA OFFLOADER_PSA_NO_REDIRECT)

zephyr_compile_options_ifdef(CONFIG_OFFLOADER_PSA
  "SHELL:$<$<COMPILE_LANGUAGE:C>:-include ${ZEPHYR_BASE}/include/zephyr/offloader/psa.h>")
