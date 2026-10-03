# Copyright (c) 2026 ITE Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_IT51XXX_HW_RSA_CALCULATION it51xxx_hw_rsa.c)
