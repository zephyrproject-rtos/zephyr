# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(video_common.c)
zephyr_library_sources(video_ctrls.c)
zephyr_library_sources(video_device.c)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_VIDEO_ARDUCAM_MEGA video_arducam_mega.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_EMUL_IMAGER video_emul_imager.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_EMUL_RX video_emul_rx.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_ESP32 video_esp32_dvp.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_GC2145 gc2145.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_HM01B0 hm01b0.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_HM0360 hm0360.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_IMX219 imx219.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_IMX335 imx335.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_MCUX_CSI video_mcux_csi.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_MCUX_MIPI_CSI2RX video_mcux_mipi_csi2rx.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_MCUX_SDMA video_mcux_smartdma.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_MT9M114 mt9m114.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV2640 ov2640.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV5640 ov5640.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV5642 ov5642.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV767X ov767x.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV7725 ov7725.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_OV9655 ov9655.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_RENESAS_RA_CEU video_renesas_ra_ceu.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_SHELL video_shell.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_STM32_DCMI video_stm32_dcmi.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_STM32_DCMIPP video_stm32_dcmipp.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_STM32_JPEG video_stm32_jpeg.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_STM32_VENC video_stm32_venc.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_ST_MIPID02 video_st_mipid02.c)
zephyr_library_sources_ifdef(CONFIG_VIDEO_SW_GENERATOR video_sw_generator.c)
# zephyr-keep-sorted-stop

if(CONFIG_VIDEO_SW_GENERATOR)
    set(VIDEO_DIR ${CMAKE_CURRENT_LIST_DIR})

    generate_inc_file_for_target(
        ${ZEPHYR_CURRENT_LIBRARY}
        ${VIDEO_DIR}/video_sw_generator_input/${CONFIG_VIDEO_SW_GENERATOR_JPEG_NAME}.jpg
        ${ZEPHYR_BINARY_DIR}/include/generated/jpeg_frame_buffer.inc
    )

    generate_inc_file_for_target(
        ${ZEPHYR_CURRENT_LIBRARY}
        ${VIDEO_DIR}/video_sw_generator_input/${CONFIG_VIDEO_SW_GENERATOR_JPEG_HFLIP_NAME}.jpg
        ${ZEPHYR_BINARY_DIR}/include/generated/jpeg_frame_buffer_hflip.inc
    )

    generate_inc_file_for_target(
        ${ZEPHYR_CURRENT_LIBRARY}
        ${VIDEO_DIR}/video_sw_generator_input/${CONFIG_VIDEO_SW_GENERATOR_PNG_NAME}.png
        ${ZEPHYR_BINARY_DIR}/include/generated/png_frame_buffer.inc
    )

    generate_inc_file_for_target(
        ${ZEPHYR_CURRENT_LIBRARY}
        ${VIDEO_DIR}/video_sw_generator_input/${CONFIG_VIDEO_SW_GENERATOR_PNG_HFLIP_NAME}.png
        ${ZEPHYR_BINARY_DIR}/include/generated/png_frame_buffer_hflip.inc
    )

    if(CONFIG_VIDEO_SW_GENERATOR_H264)
        find_program(FFMPEG_EXECUTABLE NAMES ffmpeg)
        if(NOT FFMPEG_EXECUTABLE)
            message(FATAL_ERROR "CONFIG_VIDEO_SW_GENERATOR_H264 requires ffmpeg; install ffmpeg or disable CONFIG_VIDEO_SW_GENERATOR_H264.")
        endif()

        set(H264_INPUT "testsrc2=size=${CONFIG_VIDEO_SW_GENERATOR_H264_WIDTH}x${CONFIG_VIDEO_SW_GENERATOR_H264_HEIGHT}:rate=${CONFIG_VIDEO_SW_GENERATOR_H264_FPS}")

        add_custom_command(
            OUTPUT
              ${ZEPHYR_BINARY_DIR}/include/generated/video_sw_generator_h264.h264
            COMMAND
              ${FFMPEG_EXECUTABLE} -y -f lavfi -i ${H264_INPUT} -f h264 -an
              -profile:v ${CONFIG_VIDEO_SW_GENERATOR_H264_FFMPEG_PROFILE} -pix_fmt yuv420p -level:v 3.0
              -b:v ${CONFIG_VIDEO_SW_GENERATOR_H264_BITRATE_KBPS}k
              -frames:v ${CONFIG_VIDEO_SW_GENERATOR_H264_NUM_FRAMES}
              -bsf:v h264_metadata=aud=insert
              ${ZEPHYR_BINARY_DIR}/include/generated/video_sw_generator_h264.h264
            VERBATIM
        )

        generate_inc_file_for_target(
            ${ZEPHYR_CURRENT_LIBRARY}
            ${ZEPHYR_BINARY_DIR}/include/generated/video_sw_generator_h264.h264
            ${ZEPHYR_BINARY_DIR}/include/generated/video_sw_generator_h264.inc
        )
    endif()
endif()

zephyr_linker_sources(DATA_SECTIONS video.ld)
