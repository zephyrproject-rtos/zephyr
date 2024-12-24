# Copyright (c) 2024 Plentify (Pty) Ltd
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(ade9153a.c)
zephyr_library_sources_ifdef(CONFIG_ADE9153A_TRIGGER ade9153a_trigger.c)
