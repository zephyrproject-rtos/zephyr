# Copyright (c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(akm09918c.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_AKM09918C akm09918c_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_AKM09918C .)
