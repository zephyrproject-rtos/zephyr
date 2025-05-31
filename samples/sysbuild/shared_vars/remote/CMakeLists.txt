# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# We are expecing shared variables to be present BEFORE Zephyr package is added
if(DEFINED SHARE_ME_REMOTE1)
	message(STATUS "SHARE_ME_REMOTE1=${SHARE_ME_REMOTE1}")
else()
	message(FATAL_ERROR "No SHARE_ME_REMOTE1 variable")
endif()
if(DEFINED SHARE_ME_REMOTE2)
	message(STATUS "SHARE_ME_REMOTE2=${SHARE_ME_REMOTE2}")
else()
	message(FATAL_ERROR "No SHARE_ME_REMOTE2 variable")
endif()
if(DEFINED SHARE_ME_APP1)
	message(FATAL_ERROR "Unexpected SHARE_ME_APP1 variable set")
endif()
if(DEFINED SHARE_ME_APP2)
	message(FATAL_ERROR "Unexpected SHARE_ME_APP2 variable set")
endif()

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(shared_vars)
target_sources(app PRIVATE ../src/main.c)
