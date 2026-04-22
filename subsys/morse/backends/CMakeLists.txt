# Copyright (c) 2024-2026 Freedom Veiculos Eletricos
# Copyright (c) 2024-2026 O.S. Systems Software LTDA.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DT_HAS_ZEPHYR_MORSE_GPIO_RX_ENABLED gpio_rx.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_ZEPHYR_MORSE_GPIO_TX_ENABLED gpio_tx.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_ZEPHYR_MORSE_PWM_TX_ENABLED  pwm_tx.c)
