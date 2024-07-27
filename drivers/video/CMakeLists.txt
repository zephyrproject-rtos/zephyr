# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(video_common.c)

add_subdirectory(i2c)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_VIDEO_MCUX_CSI	video_mcux_csi.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_MCUX_MIPI_CSI2RX video_mcux_mipi_csi2rx.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_STM32_DCMI	video_stm32_dcmi.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_SW_GENERATOR	video_sw_generator.c)
# zephyr-keep-sorted-stop
