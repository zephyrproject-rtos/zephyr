# SPDX-FileCopyrightText: Copyright (c) 2024 Navimatix GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

zephyr_library_sources_ifdef(CONFIG_DRV8424 drv8424.c)
