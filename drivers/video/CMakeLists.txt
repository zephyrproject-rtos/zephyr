# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(video_common.c)

zephyr_library_sources_ifdef(CONFIG_VIDEO_MCUX_CSI	video_mcux_csi.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_SW_GENERATOR	video_sw_generator.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_MT9M114	mt9m114.c)
