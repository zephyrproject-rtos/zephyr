# Copyright (c) 2025,2026 Conny Marco Menebröcker
#
# SPDX-License-Identifier: Apache-2.0
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_RFID_CR95HF rfid_cr95hf.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_CR95HF rfid_cr95hf_spi_emul.c)
