# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_VIDEO_MT9M114	video_mt9m114.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV2640	video_ov2640.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV5640	video_ov5640.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV7670	video_ov7670.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV7725	video_ov7725.c)
# zephyr-keep-sorted-stop
