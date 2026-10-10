# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(..)

zephyr_library_sources(ak09940a.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API ak09940a_async.c ak09940a_decoder.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_AK09940A ak09940a_emul.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_AK09940A . ..)
