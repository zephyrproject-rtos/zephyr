# Copyright (c) 2024 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# This script is tied to the Zephyr repository it lives in; derive ZEPHYR_BASE
# from its own location instead of trusting the environment, which may point
# at another Zephyr workspace.
cmake_path(SET ZEPHYR_BASE NORMALIZE ${CMAKE_CURRENT_LIST_DIR}/../../..)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ZEPHYR_BASE}/cmake/modules)

find_package(Zephyr-sdk COMPONENTS LIST)
