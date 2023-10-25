# Copyright (c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bma422_accel.c)
zephyr_library_sources(bma422.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_BMA422   bma422_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_BMA422 .)
