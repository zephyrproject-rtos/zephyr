# Copyright (c) 2024-2025 Freedom Veiculos Eletricos
# Copyright (c) 2024-2025 O.S. Systems Software LTDA.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DT_HAS_ZEPHYR_MORSE_GPIO_RX_ENABLED gpio_rx.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_ZEPHYR_MORSE_GPIO_TX_ENABLED gpio_tx.c)
