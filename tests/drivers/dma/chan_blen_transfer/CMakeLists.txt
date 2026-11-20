# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(chan_blen_transfer)

target_sources(app PRIVATE src/main.c)

if(CONFIG_DMA_TEST_CHANNEL_API)
  target_sources(app PRIVATE src/test_dma_channels.c)
else()
  target_sources(app PRIVATE
    src/test_dma.c
    src/test_buffers.c
  )

  if(CONFIG_DMA_LOOP_TRANSFER_RELOCATE_SECTION)
    zephyr_code_relocate(
      FILES src/test_buffers.c
      LOCATION "${CONFIG_DMA_LOOP_TRANSFER_RELOCATE_SECTION}"
    )
  endif()
endif()
