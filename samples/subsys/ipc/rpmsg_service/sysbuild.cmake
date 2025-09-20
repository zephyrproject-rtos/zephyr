cmake_minimum_required(VERSION 3.20.0)
# Copyright (c) 2019 Linaro Limited
# Copyright (c) 2018-2021 Nordic Semiconductor ASA
# Copyright (c) 2023 Arm Limited
#
# SPDX-License-Identifier: Apache-2.0
#

# default and remote board qualifier pairs
set(REMOTES
	"cpu0" "cpu1"
	"hpcore" "lpcore"
	"procpu" "appcpu"
)

while(REMOTES)
    list(POP_FRONT REMOTES FIND)
    list(POP_FRONT REMOTES REPLACE)
    string(REPLACE "${FIND}" "${REPLACE}" REMOTE_CPU "${BOARD_QUALIFIERS}")
    if(NOT ${REMOTE_CPU} STREQUAL ${BOARD_QUALIFIERS})
      break()
    endif()
endwhile()

if(${REMOTE_CPU} STREQUAL ${BOARD_QUALIFIERS})
  #message(FATAL_ERROR "BOARD_QUALIFIERS name error. Please check the target board name string.")
  set(${SB_CONFIG_RPMSG_REMOTE_BOARD} IPC_REMOTE_BOARD)
else()
  string(CONFIGURE "${BOARD}${REMOTE_CPU}" IPC_REMOTE_BOARD)
endif()

message("=> REMOTE_BOARD = ${IPC_REMOTE_BOARD}")
message("#####################")


ExternalZephyrProject_Add(
    APPLICATION rpmsg_service_remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_RPMSG_REMOTE_BOARD}
  )

add_dependencies(${DEFAULT_IMAGE} rpmsg_service_remote)
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} rpmsg_service_remote)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  # Make sure MCUboot is flashed first
  sysbuild_add_dependencies(FLASH rpmsg_service_remote mcuboot)
endif()
