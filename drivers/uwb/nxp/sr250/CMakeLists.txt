# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
    uwb_tml_sr2xx.c
    uwb_uwbs_tml_interface.c
)

zephyr_include_directories(
    .
)

zephyr_compile_definitions(CONFIG_UWBD_NAME="SR250")
