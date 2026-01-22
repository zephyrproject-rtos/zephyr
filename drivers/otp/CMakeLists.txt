# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_OTP_EMULATOR otp_emulator.c)
zephyr_library_sources_ifdef(CONFIG_OTP_SHELL otp_shell.c)
