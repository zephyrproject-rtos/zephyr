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
  /* Using mcuboot as ESP32S3 2nd stage bootloader */
  #include "mcuboot.ld"

#else

  /* Application default linker script */
  #if defined(CONFIG_SOC_ESP32S3_APPCPU)
    #include "default_appcpu.ld"
  #else
    #include "default.ld"
  #endif

#endif /* CONFIG_MCUBOOT */
