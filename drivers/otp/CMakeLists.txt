# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_OTP_EMULATOR otp_emulator.c)
zephyr_library_sources_ifdef(CONFIG_OTP_STM32_BSEC otp_bsec_stm32.c)
