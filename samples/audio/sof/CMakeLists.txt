# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(sof_module $ENV{ZEPHYR_BASE}/../modules/audio/sof)

# As a fall-back use the SOF "host" building mode for unsupported architectures.
# As more architectures begin using SOF as their audio stack we'll use a list
# of supported architectures per
# list(APPEND ...)
# if (NOT (${ARCH} IN_LIST ...))
if (NOT ${BOARD} MATCHES "^intel_adsp_cavs.*")
  set(ARCH host)
  set(OVERLAY_CONFIG ${sof_module}/src/arch/host/configs/library_defconfig)
endif ()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(sample_sof)

target_sources(app PRIVATE
    src/main.c
  )

zephyr_interface_library_named(sof_lib)

zephyr_library_include_directories(app PUBLIC
    ${sof_module}/src/arch/${ARCH}/include
    ${sof_module}/src/include
  )
