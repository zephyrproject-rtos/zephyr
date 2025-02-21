# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fs_shell)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Add a target for building the extension code
set(ext_src ${PROJECT_SOURCE_DIR}/hello_world.c)
set(ext_bin ${PROJECT_BINARY_DIR}/hello_world.llext)
add_llext_target(hello_world_ext
	OUTPUT  ${ext_bin}
	SOURCES ${ext_src}
)
