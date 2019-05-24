# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(mass)

if((NOT CONFIG_DISK_ACCESS_FLASH) AND (NOT CONFIG_DISK_ACCESS_RAM))
	message( FATAL_ERROR "No disk access settings detected. Enable one of overlays: overlay-flash-disk.conf or overlay-ram-disk.conf" )
endif()

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
