# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(video_common.c)

zephyr_library_sources_ifdef(CONFIG_VIDEO_MCUX_CSI	video_mcux_csi.c)
