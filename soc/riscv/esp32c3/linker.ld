/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 */

#if defined(CONFIG_MCUBOOT)
  /* Using mcuboot as ESP32C3 2nd stage bootloader */
  #include "mcuboot.ld"

#else
  /* Application default linker script */
  #include "default.ld"

#endif /* CONFIG_MCUBOOT */
