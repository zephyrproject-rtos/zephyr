#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mcumgr_client)

FILE(GLOB app_sources
	src/*.c
)

target_sources(app PRIVATE ${app_sources})

add_compile_definitions(CONFIG_MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER=1)
