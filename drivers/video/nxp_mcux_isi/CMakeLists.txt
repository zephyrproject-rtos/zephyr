# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(video_mcux_isi_core.c)
zephyr_library_sources(video_mcux_isi_channel.c)

zephyr_library_include_directories(.)
zephyr_library_include_directories(..)
