cmake_minimum_required(VERSION 3.20.0)
# Copyright (c) 2019 Linaro Limited
# Copyright (c) 2018-2021 Nordic Semiconductor ASA
# Copyright (c) 2023 Arm Limited
#
# SPDX-License-Identifier: Apache-2.0
#

ExternalZephyrProject_Add(
    APPLICATION rpmsg_service_remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_RPMSG_REMOTE_BOARD}
  )

add_dependencies(rpmsg_service rpmsg_service_remote)
