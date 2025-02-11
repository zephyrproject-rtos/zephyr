# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# This module is responsible for including images into sysbuild and to call
# pre and post hooks.

get_filename_component(APP_DIR  ${APP_DIR} ABSOLUTE)
get_filename_component(app_name ${APP_DIR} NAME)
set(DEFAULT_IMAGE "${app_name}")

# This is where all Zephyr applications are added to the multi-image build.
sysbuild_add_subdirectory(${sysbuild_toplevel_SOURCE_DIR}/images sysbuild/images)

get_property(IMAGES GLOBAL PROPERTY sysbuild_images)
sysbuild_module_call(PRE_CMAKE MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES})
sysbuild_images_order(IMAGES_CONFIGURATION_ORDER CONFIGURE IMAGES ${IMAGES})
foreach(image ${IMAGES_CONFIGURATION_ORDER})
  sysbuild_module_call(PRE_IMAGE_CMAKE MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES} IMAGE ${image})
  ExternalZephyrProject_Cmake(APPLICATION ${image})
  sysbuild_module_call(POST_IMAGE_CMAKE MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES} IMAGE ${image})
endforeach()
sysbuild_module_call(POST_CMAKE MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES})

sysbuild_module_call(PRE_DOMAINS MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES})
include(${sysbuild_toplevel_SOURCE_DIR}/cmake/domains.cmake)
sysbuild_module_call(POST_DOMAINS MODULES ${SYSBUILD_MODULE_NAMES} IMAGES ${IMAGES})
