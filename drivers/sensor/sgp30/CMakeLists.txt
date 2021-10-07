# Copyright (c) 2021 Advanced Climate Systems
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SGP30 sgp30.c)
