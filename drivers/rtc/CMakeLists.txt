# Copyright (c) 2022 Bjarki Arge Andreasen
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE rtc_handlers.c)
zephyr_library_sources_ifdef(CONFIG_RTC_EMUL rtc_emul.c)
