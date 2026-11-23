# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(fota_http.c)
zephyr_library_sources_ifdef(CONFIG_FOTA_HTTP_SHELL shell.c)
