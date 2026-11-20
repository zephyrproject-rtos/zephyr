# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_H264SWDEC)
  set(H264SWDEC_DIR ${ZEPHYR_H264SWDEC_MODULE_DIR})

  zephyr_include_directories(${H264SWDEC_DIR}/inc)

  zephyr_library()
  zephyr_library_sources(
    ${H264SWDEC_DIR}/source/h264bsd_transform.c
    ${H264SWDEC_DIR}/source/h264bsd_util.c
    ${H264SWDEC_DIR}/source/h264bsd_byte_stream.c
    ${H264SWDEC_DIR}/source/h264bsd_seq_param_set.c
    ${H264SWDEC_DIR}/source/h264bsd_pic_param_set.c
    ${H264SWDEC_DIR}/source/h264bsd_slice_header.c
    ${H264SWDEC_DIR}/source/h264bsd_slice_data.c
    ${H264SWDEC_DIR}/source/h264bsd_macroblock_layer.c
    ${H264SWDEC_DIR}/source/h264bsd_stream.c
    ${H264SWDEC_DIR}/source/h264bsd_vlc.c
    ${H264SWDEC_DIR}/source/h264bsd_cavlc.c
    ${H264SWDEC_DIR}/source/h264bsd_nal_unit.c
    ${H264SWDEC_DIR}/source/h264bsd_neighbour.c
    ${H264SWDEC_DIR}/source/h264bsd_storage.c
    ${H264SWDEC_DIR}/source/h264bsd_slice_group_map.c
    ${H264SWDEC_DIR}/source/h264bsd_intra_prediction.c
    ${H264SWDEC_DIR}/source/h264bsd_inter_prediction.c
    ${H264SWDEC_DIR}/source/h264bsd_reconstruct.c
    ${H264SWDEC_DIR}/source/h264bsd_dpb.c
    ${H264SWDEC_DIR}/source/h264bsd_image.c
    ${H264SWDEC_DIR}/source/h264bsd_deblocking.c
    ${H264SWDEC_DIR}/source/h264bsd_conceal.c
    ${H264SWDEC_DIR}/source/h264bsd_vui.c
    ${H264SWDEC_DIR}/source/h264bsd_pic_order_cnt.c
    ${H264SWDEC_DIR}/source/h264bsd_decoder.c
    ${H264SWDEC_DIR}/source/H264SwDecApi.c
    ${H264SWDEC_DIR}/source/H264SwDecAllocator.c
    zephyr_h264swdec.c
  )
  zephyr_library_include_directories(${H264SWDEC_DIR}/source)
  zephyr_library_compile_definitions(
    H264SWDEC_CUSTOM_ALLOCATOR=1
    H264SWDEC_MAX_FRAME_WIDTH=${CONFIG_H264SWDEC_MAX_FRAME_WIDTH}
    H264SWDEC_MAX_FRAME_HEIGHT=${CONFIG_H264SWDEC_MAX_FRAME_HEIGHT}
    H264SWDEC_MAX_FRAME_SIZE=${CONFIG_H264SWDEC_MAX_FRAME_SIZE}
  )
  # The decoder is C99 code imported from Android.
  zephyr_library_compile_options(
    -Wno-maybe-uninitialized
    -Wno-unused-but-set-variable
    -Wno-unused-parameter
    -Wno-unused-variable
  )
endif()
